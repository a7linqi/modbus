# F407 网关架构设计 + 三端一致性契约 + 里程碑门禁

> 角色：系统架构师交付物（2026-11-26 冻结）。与 `AI-HANDOFF.md` v1.1 配套；冲突时以本文的细化为准，范围冲突以冻结 v1 为准。
> 本文回答四件事：F407 内部怎么跑（任务/栈/队列/阻塞/ISR 边界）、三端字节与语义怎么保证唯一版本、每个里程碑怎么验收、架构审查查什么。

---

## 0. 范围裁决（对外部建议的采纳与拒绝）

| 外部建议 | 裁决 | 理由 |
|---|---|---|
| 首版单从站 + 6 寄存器最小表 | **拒绝**，保持 4 逻辑从站 18 点 | 冻结表已评审；调度/镜像/离线降频的演示价值依赖 ≥2 从站。W1 首个闭环仍只读 Slave 1 的 4 个输入寄存器，与"最小起步"精神一致 |
| 单设备定长 `GatewayPacket`（温度/电压/电流打包） | **拒绝**，保持 26B 头 + 类型化批量 payload | 单设备包在 W4 加从站、加报警/命令时必然重写协议——正是冻结要防的返工。批量点位格式见 §3 |
| M4 先做 LwIP 再做 Linux | **拒绝**，LwIP 保持 W6 可选 | Linux 服务可用 gateway_simulator / 串口桥先验证，PHY bring-up 风险不阻塞主线 |
| 按功能拆任务（Poll/Rx/Process/NetTx/Health） | **拒绝**，保持冻结 5 任务 | Rx 留在 ISR+port 层，解析在 Transport 任务内；"一个功能一个任务"会造成任务过多、接口混乱 |
| 加 MQTT/TLS/OTA/Web/Docker/云 | **拒绝** | 已在 AI-HANDOFF §3.3 明确不做 |
| 模型分工（架构/执行/审查/文档分离） | **采纳** | 见 `agent-workflow.md` §10 |
| 任务单限制允许/禁止修改文件 + 边界条件 + DoD | **采纳** | 见 `docs/task-tickets.md` |

---

## 1. F407 任务架构（细化冻结 §6.5）

全部任务/队列**静态创建**（`xTaskCreateStatic` / `xQueueCreateStatic`），全工程无 malloc。F407ZGT6 192KB SRAM，预算充裕但必须可解释。

| 任务 | 优先级 | 栈初值（word） | 周期/触发 | 最大阻塞 | 职责 |
|---|---|---|---|---|---|
| Rs485TransportTask | 5 | 512 | RequestQueue 到达 | 等请求 ≤10ms；等响应 ≤50ms（状态机超时，非 vTaskDelay） | 独占 UART；执行事务状态机；DE/TC 时序 |
| PollingSchedulerTask | 4 | 384 | 10ms tick | ≤10ms | 到期生成读请求；写请求优先；离线降频 |
| RegisterModelTask | 3 | 384 | ResultQueue 到达 | ≤20ms | 更新镜像/有效期/质量；边缘闭环判断；发布快照 |
| NetworkTask | 3 | 768（LwIP 启用后 1024） | SnapshotQueue / socket 事件 | socket 超时 100ms | 上报/收命令；W6 前可禁用 |
| HealthMonitorTask | 1 | 256 | 100ms | ≤100ms | 心跳位图、栈水位/队列峰值统计、喂狗 |

优先级数值的解释：Transport 最高保证总线事务不被打断；Scheduler 次之保证节拍；Register/Network 同级靠队列解耦；Health 最低但喂狗逻辑独立于业务优先级。`configMAX_PRIORITIES = 7`。

栈初值依据：热路径不 printf、不递归、局部数组 ≤256B；日志只在 Health 用固定格式 snprintf 到静态缓冲。**W3 必须记录 `uxTaskGetStackHighWaterMark()` 实测值并据此调整**，调整理由写进测试报告。

### 1.1 队列与内存池（全部静态）

| 对象 | 深度/容量 | 元素 | 满策略 |
|---|---|---|---|
| RequestQueue | 16 | `ModbusRequest`（≈24B） | 丢 newest + 计数（写请求单独高优先通道，见 §1.3） |
| ResultQueue | 16 | `ModbusResult`（≈32B） | 丢 oldest + 计数 |
| FrameQueue（ISR→Transport） | 4 | 帧池指针 | 丢 oldest + 计数 |
| 帧池 | 4 × 256B | 静态数组 | — |
| SnapshotQueue | 8 | 快照池指针 | 丢 oldest + 计数 |
| 快照池 | 8 × 512B | 静态数组 | — |

### 1.2 同步原语选择（为什么不用 mutex）

| 资源 | 原语 | 理由 |
|---|---|---|
| UART/RS485 | **单所有者**（Transport），无 mutex | 冻结"禁止多任务访问 UART"；所有权比锁更简单且无死锁面 |
| 寄存器镜像 | **单写者**（Register）+ 快照池发布 | 读者只拿快照指针，不共享可变状态 → 无需 mutex，无半新半旧 |
| ISR→Transport | Queue（FromISR）+ TC 用 task notification | 队列传帧指针不拷贝 256B；notification 零开销唤醒 |
| 任务心跳 | Event group 位图 | 5 个心跳位，Health 等"全新鲜"才喂狗 |
| 写请求优先 | 独立 WriteQueue（深度 4）+ Transport 先查 WriteQueue | 比优先级继承 mutex 简单，语义明确 |

### 1.3 ISR 与任务边界（硬约束）

ISR 只做：DMA 半满/完成标志、IDLE 判帧（读 NDTR 算长度→帧指针入 FrameQueue→重启 DMA）、TC 中断里**单条 GPIO 写**拉低 DE。
ISR 不做：CRC、协议解析、malloc、printf、HAL_Delay、队列阻塞等待。
解析/校验/重试全部在 Transport 任务内。

### 1.4 事务错误路径

| 错误 | 动作 | 计数 | 镜像质量 |
|---|---|---|---|
| 响应超时（50ms） | retry_count<2 → 请求放队尾；否则记一次失败 | timeout++ | 2 通信错 |
| CRC 错 | 同上重试路径 | crc_err++ | 2 |
| 异常响应（0x01/02/03/04） | **不重试**，直接上报异常码 | exception++ | 3 越限/按码 |
| 连续失败 3 次 | 标离线，轮询降频 2s/5s/10s | offline++ | 1 过期 |
| 连续成功 2 次 | 恢复在线，恢复原周期 | recovery++ | 0 正常 |
| 队列满 | 丢帧/丢请求 + 计数，不阻塞 | overflow++ | 不变 |

看门狗：IWDG ≈2s；Health 每 100ms 收心跳位，全部位新鲜度 <500ms 才喂；任一任务卡死 → 复位，复位原因入统计区。

---

## 2. 三端一致性契约（Modbus 域）

- 唯一来源 `docs/register-map.csv` → 生成器产出 C/Python/Markdown；**任何一端手写寄存器地址即违规**。
- 寄存器域字节序：Modbus 规范大端；uint32 = ABCD、偶数对齐（冻结）。
- 地址 0-based；0x03/0x04 空间独立；错空间 → 异常 0x02。
- Point ID = `(slave_id<<8)|(space<<7)|index`，三端共用同一编码（生成器输出）。
- 缩放只在 register-map 定义；F103 存原始整数，Linux 落库带 scale 列，**换算不在 MCU 做浮点**。

## 3. 三端一致性契约（应用层域，冻结 payload 格式）

头沿用冻结 §6.6：26B、`pack(1)`、**多字节字段大端（网络序）**、header_crc 覆盖前 24B、payload_crc 覆盖 payload、CRC-16/CCITT-FALSE。两端必须 `static_assert(sizeof(header)==26)`，收发统一走 hton/ntoh 封装函数，禁止直接 memcpy 结构体上线。

payload 格式（本次冻结，取代"W5 前再定"）：

```c
/* MSG_DEVICE_DATA：批量点位 */
uint16_t point_count;
struct PointEntry {          /* 16B，大端 */
    uint16_t point_id;
    uint8_t  quality;        /* 0正常 1过期 2通信错 3越限 */
    uint8_t  size;           /* 1 或 2 个寄存器 */
    uint32_t value_raw;      /* 16位点按 map 符号/零扩展 */
    uint32_t timestamp_ms;   /* 网关 uptime */
    uint16_t reserved;       /* 0 */
};

/* MSG_DEVICE_STATUS */
uint16_t slave_count;
struct SlaveStatus {         /* 12B */
    uint8_t  slave_id; uint8_t online;
    uint16_t consecutive_failures; uint16_t last_error;
    uint32_t last_success_ms;
};

/* MSG_ALARM：16B */
uint16_t point_id; uint8_t alarm_type; uint8_t severity;
uint32_t value_raw; uint32_t timestamp_ms; uint16_t reserved;

/* MSG_COMMAND：8B */
uint16_t point_id; uint16_t value; uint32_t command_id;
/* MSG_COMMAND_RESPONSE：8B */
uint32_t command_id; uint8_t result; /*0ok 1异常 2超时 3离线*/
uint8_t modbus_exception; uint16_t reserved;

/* MSG_HELLO：8B */  uint8_t fw_major, fw_minor; uint16_t point_count; uint32_t uptime_ms;
/* MSG_HEARTBEAT：8B */ uint32_t uptime_ms; uint16_t queue_peak; uint16_t stack_min_words;
```

语义规则：

- sequence 每连接从 1；Linux 检测 gap → 记丢包统计，**不重传**（遥测语义）；重复/乱序按无符号回绕处理。
- version 不支持、magic 错、长度超限、任一 CRC 错 → 断连 + 日志（冻结）。
- 升级兼容：v1 内只允许启用 reserved/flags 位，**不允许改字段顺序/宽度**；改布局必须 version=2 且 Linux 双版本解析过渡。
- Linux 每连接静态接收环 2048B（最大帧 26+1024=1050B），连接池静态 8 槽；半包/粘包状态机：找 magic → 收满 26B → 验 header_crc → 按 length 收 payload → 验 payload_crc → 分发。
- 命令闭环：Linux 发 COMMAND → F407 写从站 → 收到 0x06/0x10 回显才发 COMMAND_RESPONSE(result=0) 并更新镜像（"确认后更新"语义）。
- 入库变化阈值（冻结）：模拟量 ±5 raw、电量 ±1、离散量任何变化；与 1s 下限取 AND；详见 AI-HANDOFF §6.7。

---

## 4. 里程碑门禁（G1–G5）与降级路径

| 门禁 | 时点 | 通过标准 | 未通过降级 |
|---|---|---|---|
| G1 | W1 末 12/20 | PC↔F103：0x03/0x04 各 1000 次 0 错；写回显；0x02 异常 | 功能码缩到 0x03+0x06，照常进 W2 |
| G2 | W3 末 01/03 | F407 单从站 4 功能码；5 任务运行；HWM/队列峰值有记录 | 退单任务状态机版（无 FreeRTOS），仍可写简历 |
| G3 | W4 末 01/10 | 4 从站调度；拔一从站不影响其他；恢复自动上线；边缘闭环动作正确 | Linux 端降级 Python 原型，简历改"Python 原型验证" |
| G4 | W5 末 01/17 | Linux 按变化触发+1s+保留期入库；断线重连不丢数据 | 保留 SQLite 设计文档+模拟器证据，C 服务延后 |
| G5 | W6 末 01/24 | 20 故障场景 + 24h 无复位 | 砍 LwIP/24h，保 20 场景与演示视频 |

规则：任何门禁未通过**只降级、不回头重设计**；降级决定记录进 w1-status 同类周报。

## 5. 里程碑架构审查清单（每门禁跑一次，只看头文件+关键 .c+日志）

1. 实现是否偏离本文与冻结 v1（任务数、队列、协议字段）？
2. 接口是否重复定义（寄存器地址/point_id/CRC 函数出现第二份手写）？
3. ISR 是否越界（解析/CRC/阻塞/分配出现在中断）？
4. 是否存在 malloc/new/动态栈数组？
5. 阻塞是否有上界（每个等待点有 timeout）？单所有者/单写者是否被破坏？
6. 错误路径是否完整（§1.4 表逐条对照代码）？
7. 字节序是否统一走封装（grep 直接 memcpy 结构体上线）？
8. 队列满/丢帧是否有计数且被测试覆盖？
9. 文档与代码是否一致（寄存器表、payload 结构、栈/队列数值）？
10. 下一里程碑的输入是否就绪（接口冻结、测试基线通过）？

审查输出：偏离清单 + 是否放行下一里程碑。审查者不修改代码。
