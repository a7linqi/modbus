# 工业边缘通信网关项目 · AI 交接文档

> 用途：把这份文档直接发给另一个 AI 对话，让它具备完整上下文，不需要你再逐条解释。
> 项目状态：未开工。第一里程碑是 F103 Modbus RTU 从站 0x03 最小闭环。
> 关键约定（v1 冻结）：RS485 115200 8N1；4 个逻辑从站（3 业务 + 1 预留）；测量值走 0x04、参数/控制走 0x03；应用层协议头 26 字节；SQLite 按“变化触发 + 1s 下限 + 保留期”入库。

---

## 0. 给 AI 的启动语（可直接复制）

> 你是我的嵌入式项目结对工程师。请先完整阅读这份交接文档，然后只执行“第一周任务”。不要一次性生成整个项目；每次只做一个模块，先给接口、状态机、错误码和测试用例，再给代码。遇到硬件引脚、UART 实例、时钟配置不明确时，先问我，不要猜。

---

## 1. 项目一句话

基于 FreeRTOS 与 Linux 的工业边缘通信系统：

**STM32F103（Modbus RTU 从站）→ RS485 → STM32F407（Modbus RTU 主站/网关）→ 以太网 → Linux C 服务（epoll + SQLite）**

目标：做一个能写进简历、能演示、能面试讲清 5 分钟的完整嵌入式项目。

---

## 2. 用户背景与现有条件

### 2.1 技术背景

- 2027 届本科，自动化专业
- 熟悉 C、STM32 HAL、FreeRTOS、CAN/ISO-TP/UDS、Bootloader/OTA
- Linux 初学者，会基本命令，没写过 Linux 网络服务
- 会用 Keil、CubeMX、ST-Link、逻辑分析仪、示波器
- 有 AI 辅助开发经验，但希望本项目**核心模块自己写、能讲清**
- 简历实读（2026-11-26）：已有 3 个量化项目（CAN/UDS+Ed25519 OTA 平台、BLE 手表无线 OTA、FreeRTOS+MQTT 灌溉终端）与汽车零件供应商软件验证实习（TESSY）；心跳+IWDG、队列解耦、快照保护、回差、CRC 状态机在旧项目均有实测背书
- 本项目定位是第 4 个项目/替换位，不是从 0 到 1；简历条目避免重复旧项目亮点，聚焦 Modbus 标准协议、多从站调度、Linux 服务层（详见 `docs/resume-draft.md` 定位节）

### 2.2 现有硬件

| 硬件 | 说明 |
|---|---|
| STM32F407 探索者 V3 | 板载 **YT8512C** 以太网 PHY、**TPT8485** RS485（与 SP3485 引脚兼容）、TJA1050 CAN |
| STM32F103 开发板 | 作为 Modbus RTU 从站 |
| USB-RS485 转换器 | PC 调试 / 模拟从站 |
| 120Ω 电阻 | RS485 总线终端 |
| 面包板、杜邦线、双绞线 | 接线 |
| 电位器 | 接 PA0（ADC1_IN0），两端 3.3V/GND、抽头 PA0 → Slave 2 电压真实采集 |
| AHT20 温湿度模块 | I2C1 PB6 SCL / PB7 SDA，3.3V，地址 0x38 → Slave 1 温度真实采集 |
| PC / Linux VM | Linux 服务、Python 工具 |

### 2.3 重要纠正

- **不要写 LAN8720**：F407 板载是 **YT8512C**（裕太微 RMII PHY）。
- **F407 板载 RS485 是 TPT8485**（不是 SP3485，两者引脚兼容）：USART2 PA2/PA3，方向脚 **PG8 高 = 发送、低 = 接收**（DE 与 /RE 短接）。
- **P4 跳线决定 PA2/PA3 走 RS232 还是 RS485**；跳错时串口无响应且代码看不出问题。
- **不要默认用 C++**：Linux 端用 C，除非用户明确要求 C++。
- 不要假设 F103 是 5V 逻辑；RS485 模块必须 **3.3V**。
- **RS485 固定 115200 8N1**：不再以 9600/19200 作为主线；115200 下必须用 UART IDLE/DMA 判帧。
- **逻辑从站固定 4 个**：Slave 1 温度/压力、Slave 2 电量、Slave 3 执行器、Slave 4 预留/测试。
- **寄存器空间固定**：测量值走 0x04 输入寄存器，参数/控制/状态走 0x03 保持寄存器；地址一律 0-based。
- 不要编造实测数字；所有数字必须用户实测后填写。

### 2.4 时间约束

- 2026-12-12 考 CET-4，之前每天最多 1 小时做项目，只做文档/CRC16/寄存器表。
- 12/13 后每天 2–3 小时，按 6 周计划推进。
- 2027 年 2–4 月春招，项目必须在 1 月底前形成可写简历的 MVP。

---

## 3. 项目目标与非目标

### 3.1 必须完成（MVP）

- F103：Modbus RTU 从站（4 个逻辑 Slave ID），0x03/0x04/0x06/0x10、CRC16、异常响应、故障注入
- F407：Modbus RTU 主站、非阻塞状态机、4 从站调度、超时重试、离线降频、恢复上线、寄存器镜像
- F407 边缘控制闭环：温度阈值 + 回差自动控制执行器，云端断连不影响本地控制（2026-11-25 确认加入）
- Linux：C + epoll TCP 服务、半包/粘包处理、心跳、SQLite 持久化、日志
- Python：主站/从站模拟器、TCP 网关模拟器、故障注入、测试报告
- 测试：正常读写、CRC 错误、超时、非法地址/功能码、断线重连、24 小时稳定性

### 3.2 第二阶段（可选）

- 双区配置与掉电恢复
- F407 LwIP + 以太网 TCP 客户端
- Modbus TCP Server
- 72 小时稳定性、30+ 故障场景
- systemd、Web/Qt 界面、MQTT、CANopen

### 3.3 明确不做

- GUI/仪表盘
- DoIP、车载以太网、AUTOSAR
- 云平台、多网关集群
- 复杂前端、数据库集群
- 为了关键词强行上 C++/systemd

---

## 4. 系统架构

```
F103 从站（4 个逻辑 Slave ID，单板模拟）
├── Slave 1：温度/压力传感器
├── Slave 2：电量采集
├── Slave 3：执行器
└── Slave 4：预留/测试从站
        │
        │ RS485 / Modbus RTU（固定 115200 8N1）
        ▼
F407 网关（FreeRTOS）
├── Rs485TransportTask（独占 UART/RS485）
├── PollingSchedulerTask（周期/优先级/离线降频）
├── RegisterModelTask（寄存器镜像/有效期/权限）
├── NetworkTask（LwIP TCP 客户端，可选）
└── HealthMonitorTask（心跳/栈余量/喂狗）
        │
        │ 以太网（YT8512C）或 PC 模拟器/串口桥
        ▼
Linux 服务（C）
├── epoll 事件循环
├── 连接管理/心跳/断线重连
├── 应用层拆包/粘包处理
├── SQLite 批量写入
└── 日志/统计
        ▲
        │
Python 工具：Modbus 从站模拟、故障注入、测试报告
```

---

## 5. 硬件与接线

### 5.1 RS485

- 接线：F407 板载 RS485 收发器 **TPT8485**（与 SP3485/MAX3485 引脚兼容）的 A/B/GND ↔ F103 RS485 模块 A/B/GND
- F407 侧已核实，详见 `docs/hardware-f407-v3.md`：
  - **USART2：PA2 = TX → DI，PA3 = RX ← RO**
  - 方向脚 **PG8**，DE 与 /RE 短接：**高 = 发送，低 = 接收**（单脚控制，不是自动收发）
  - **P4 跳线帽必须拨到 RS485 侧**；PA2/PA3 同时接 RS232（COM2），跳错时串口完全无响应且代码看不出问题
  - 板载 A/B 间已有 **120Ω**（R30）+ 360Ω 上下拉偏置
  - **PG8 与 NRF24L01 的 IRQ 共用**，本项目不插 NRF 模块
- 固定 **115200 8N1**，F103、F407、USB-RS485 三端必须一致
- F103 侧模块必须是 **3.3V 逻辑**，并确认 DE/RE 控制方式（GPIO 控制或自动收发），见 `docs/w1-status.md` 硬件 3 问
- 发送前拉高 DE（F407 即 PG8），DMA 发送完成后继续等 **UART TC**，才能拉低 DE 回接收
- 终端：F407 端已板载 120Ω，F103 端再加一个 120Ω 即构成两端；短距离桌面可先只保留板载端
- 使用双绞线，A/B 不要接反；共地
- 调试时可用 USB-RS485 接 PC，先验证 F103 从站

> 原理图网络名与 MCU 功能名相反：`RS485_RX` 接 DI（MCU 的 TX），`RS485_TX` 接 RO（MCU 的 RX），不要按网络名接线。

### 5.2 以太网

- F407 板载 YT8512C，RMII 接口
- 网线接 PC/路由器；LwIP 阶段再启用
- 若 LwIP 卡住，允许先用 PC 模拟器或串口桥替代，不阻塞 MCU MVP

### 5.3 调试工具

- 逻辑分析仪：看 UART TX、DE/RE、A/B 差分
- USB-RS485：PC 端 Modbus 主站/从站模拟
- ST-Link：下载与调试
- 示波器：看 RS485 波形与终端匹配

---

## 6. 详细需求

### 6.1 F103 Modbus RTU 从站

#### 寄存器表与空间约定（最终以 docs/register-map.md 为准）

- 地址一律 **0-based** 协议地址，不使用 4xxxx 表示法。
- 测量值放 **0x04 输入寄存器**（只读）；参数、控制、状态放 **0x03 保持寄存器**（R/W 或 W）。
- 0x03 与 0x04 是两个独立地址空间，同一数值地址可以同时存在。
- uint32 统一大端 **ABCD**（高字在前），起始地址偶数对齐。
- 每个逻辑从站都有 `0x00F0` 故障注入寄存器，定义见下。
- 每个从站的测量点、参数点、缩放和镜像地址以 `docs/register-map.md` 为唯一来源。

**Slave 1：温度与压力**

0x04 输入寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0000 | 设备状态 | uint16 | R |
| 0x0001 | 温度 ×10 | int16 | R |
| 0x0002 | 压力 ×100 | uint16 | R |
| 0x0003 | 报警状态 | uint16 | R |

0x03 保持寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0010 | 采样周期 ms（默认 200） | uint16 | R/W |
| 0x00F0 | 故障注入模式 | uint16 | R/W |

**Slave 2：电量采集**

0x04 输入寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0000 | 电压 ×10 | uint16 | R |
| 0x0001 | 电流 ×100 | uint16 | R |
| 0x0002–0x0003 | 累计电量 uint32（ABCD） | uint32 | R |

0x03 保持寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0010 | 清零命令（写 1 清零，读恒为 0） | uint16 | RW |
| 0x00F0 | 故障注入模式 | uint16 | R/W |

**Slave 3：执行器**

0x04 输入寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0000 | 输出状态 | uint16 | R |

0x03 保持寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0010 | 输出控制 | uint16 | R/W |
| 0x0011 | 目标值 | uint16 | R/W |
| 0x00F0 | 故障注入模式 | uint16 | R/W |

**Slave 4：预留/测试**

0x04 输入寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0000 | 运行计数（每 200ms +1） | uint16 | R |

0x03 保持寄存器：

| 地址 | 含义 | 类型 | 权限 |
|---|---|---|---|
| 0x0010 | 测试写入值 | uint16 | R/W |
| 0x00F0 | 故障注入模式 | uint16 | R/W |

> 只读测量量不得通过 0x03 读取，参数/控制不得通过 0x04 读取；用错空间返回异常码 0x02。

#### 功能码与协议常量

- 0x03 读保持寄存器：1–125 个
- 0x04 读输入寄存器：1–125 个
- 0x06 写单个寄存器：回显地址和值
- 0x10 写多个寄存器：1–123 个，响应回显起始地址和数量
- 从站地址 1–247；0 为广播（只执行写，不响应）；248–255 保留
- ADU 最大 256 字节；固定收发缓冲区各 256 字节
- 异常响应固定为：功能码 | 0x80，后跟 1 字节异常码
- CRC16：多项式 0xA001（Modbus），初值 0xFFFF，低字节在前；测试向量 `"123456789"` → 0x4B37

#### 异常码（Modbus 异常码，与寄存器空间 0x03/0x04 无关）

- 0x01 非法功能码
- 0x02 非法数据地址
- 0x03 非法数据值
- 0x04 从站设备故障

#### 故障注入

```c
typedef enum {
    FAULT_NONE = 0,
    FAULT_NO_RESPONSE,
    FAULT_BAD_CRC,
    FAULT_DELAY_RESPONSE,
    FAULT_EXCEPTION_CODE,
    FAULT_TRUNCATED_FRAME
} FaultMode;
```

- 4 个逻辑从站的 `0x00F0` 都可写上述枚举，只影响当前从站。
- `FAULT_NO_RESPONSE` 生效后 Modbus 无法再写回清除，因此注入 **30 秒后自动恢复 `FAULT_NONE`**；另提供调试串口命令立即清除。
- `FAULT_DELAY_RESPONSE` 默认延迟 500ms，用于主站超时测试，可通过调试串口修改。
- 故障注入不得导致看门狗复位或影响其他从站。

#### 实现约束

- 无动态内存；固定 256 字节收发缓冲区
- 115200 下 3.5 字符间隔约 0.3ms，**必须用 UART IDLE/DMA 判帧**，不能靠 1ms 定时器
- F103 侧 DE/RE：发送前拉高 DE，DMA 完成后继续等 UART TC，再拉低 DE 回接收
- 校验从站地址、功能码、帧长度、寄存器空间、寄存器范围、CRC16
- CRC16 低字节在前（Modbus RTU）
- 可配置响应延迟，用于主站超时测试
- 裸机主循环用 SysTick 做非阻塞节拍；`HAL_Delay()` 只允许在初始化阶段使用
- 代码分模块，不要全写在 main.c

#### 验收

- PC 发送合法 0x03/0x04 请求，返回正确寄存器
- 0x06/0x10 写入成功并正确回显
- 非法地址返回 0x02 异常；CRC 错帧丢弃并计数
- 115200 下 0x03/0x04 各连续读 1000 次无错误
- 故障注入 30 秒后自动恢复

---

### 6.2 F407 Modbus RTU 主站

#### 事务状态机

```c
typedef enum {
    MB_TRANS_IDLE = 0,
    MB_TRANS_BUILDING,
    MB_TRANS_TRANSMITTING,
    MB_TRANS_WAIT_TX_COMPLETE,
    MB_TRANS_WAIT_RESPONSE,
    MB_TRANS_VALIDATING,
    MB_TRANS_COMPLETED,
    MB_TRANS_RETRY,
    MB_TRANS_FAILED
} ModbusTransactionState;

typedef struct {
    uint8_t slave_id;
    uint8_t function_code;
    uint16_t start_address;
    uint16_t quantity;
    uint16_t timeout_ms;
    uint8_t retry_max;
    uint8_t retry_count;
    uint32_t request_timestamp;
    uint32_t response_timestamp;
    ModbusTransactionState state;
    uint8_t error_code;
} ModbusTransaction;
```

#### RS485 DE/RE 时序（必须正确）

1. 拉高 DE，进入发送
2. 启动 UART DMA 发送
3. 等待 DMA 完成（数据搬到 UART）
4. **继续等待 UART TC 标志**（最后一个停止位发完）
5. 拉低 DE，进入接收
6. 启动响应超时计时

> DMA 完成 ≠ 最后一个字节发完；DE 切早了会截断最后一字节。

- 固定 115200 8N1；20 个寄存器一次事务约 5ms，超时默认 50ms（约 10 倍帧时间余量）。
- DE 切换由 UART TC 中断/标志驱动，不要用 `HAL_Delay()` 轮询。

#### 非阻塞要求

- 禁止 `send_request(); HAL_Delay(100); wait_response();`
- 用状态机 + 系统 tick/定时器
- 一次只允许一个事务占用 RS485

#### 验收

- 115200 下单从站 0x03/0x04/0x06/0x10 正常
- 拔线后超时、重试、失败计数正确
- 通信期间其他任务正常运行

---

### 6.3 多从站调度器

```c
typedef struct {
    uint8_t slave_id;
    uint8_t function_code;
    uint16_t start_address;
    uint16_t quantity;
    uint32_t poll_period_ms;
    uint32_t next_poll_time;
    uint16_t timeout_ms;
    uint8_t retry_max;
    uint8_t offline_threshold;
    uint8_t recovery_threshold;
    uint8_t consecutive_failures;
    uint8_t consecutive_successes;
    bool online;
} PollItem;
```

#### 调度规则

- 正常设备按各自周期轮询
- 写请求优先于普通采集
- 连续失败 3 次标记离线
- 离线后轮询间隔从 500ms 拉长到 5s（不同设备可配 2s/5s/10s）
- 连续成功 2 次恢复在线
- 一次只允许一个 Modbus 事务
- 超时/CRC 错误后不原地阻塞重试：事务进入 RETRY，把请求放回队列尾部，先服务其他到期设备
- 单次读取数量默认 ≤20；0x03/0x04 的 125 个只是协议理论上限
- `retry_max = 2` 表示首次失败后最多再重试 2 次，仍失败才计入连续失败
- 队列满必须统计并记录
- 一个故障设备不能阻塞其他设备

#### 示例（115200，单次读取 ≤20 个寄存器）

| 设备 | 周期 | 超时 | 重试 | 失败阈值 | 离线周期 |
|---|---|---|---|---|---|
| 温度设备（Slave 1） | 500ms | 50ms | 2 | 3 | 5s |
| 电量设备（Slave 2） | 1000ms | 50ms | 2 | 3 | 10s |
| 执行器状态（Slave 3） | 200ms | 50ms | 2 | 3 | 2s |
| 预留从站（Slave 4） | 1000ms | 50ms | 2 | 3 | 5s |

> 50ms 是按 20 寄存器响应约 4ms 留出的约 10 倍余量；单次读取超过 20 个寄存器会显著增加帧时间，必须重新核算超时。

#### 验收

- 关闭一个逻辑从站，其他从站继续运行
- 恢复后自动重新上线

---

### 6.4 寄存器镜像

```c
typedef struct {
    uint32_t value;           // 16 位点符号/零扩展后统一存 uint32
    uint8_t  size;            // 1 = 16 位，2 = 32 位
    uint8_t  space;           // 0 = 保持寄存器，1 = 输入寄存器
    uint16_t source_address;  // 从站起始地址
    uint32_t timestamp_ms;
    uint32_t valid_duration_ms;
    bool valid;
    bool writable;
} RegisterEntry;
```

- 一个 `RegisterEntry` 对应一个逻辑点，不对应单个 16 位寄存器；32 位值一次性更新，避免半新半旧。
- **控制类点遵循"确认后更新"**：写命令发出 ≠ 生效，收到 0x06/0x10 回显确认后才更新镜像值并标 valid，区分"期望值"与"实际值"（借鉴 R1 简历的状态一致性原则）。
- 32 位数据统一大端 ABCD；合法范围、缩放、更新时间、是否过期都写在 `docs/register-map.md`。
- 镜像地址映射固定为：测量/控制/统计分区 = 分区基址 + `(slave_id - 1) * 0x40` + 点内偏移；状态区每从站 0x10；0x0400 健康区为全局。点内偏移是 register-map.md 分配的紧凑索引（0x00–0x3F），不是从站本地寄存器地址。
- 地址分区：
  - 0x0000–0x003F：4 个从站在线状态与错误码，每从站 0x10
  - 0x0040–0x00FF：保留
  - 0x0100–0x01FF：测量值，每从站 0x40
  - 0x0200–0x02FF：控制/参数，每从站 0x40
  - 0x0300–0x03FF：通信统计，每从站 0x40
  - 0x0400–0x04FF：任务与系统健康（全局）

---

#### 边缘控制闭环（W4，2026-11-25 确认加入）

- 规则：Slave 1 温度 ≥ 30.0°C（寄存器值 300）→ 自动写 Slave 3 `0x0010 = 1`；≤ 28.0°C（280）→ 自动写 `0x0010 = 0`。**回差 2°C**，防止阈值附近抖动。
- 实现位置：F407 `RegisterModelTask` 内做规则判断，复用既有写请求队列下发，**不新增任务**。
- 约束：只在 Slave 3 在线且温度点 valid 时动作；状态变化才写，不周期性重复写；云端断连不影响本地闭环。
- 叙事点：网关不只是协议转换器——本地闭环保证断网时现场仍可控，这是"边缘"二字的价值。
- 面试点：回差（hysteresis）为什么必须有；为什么放边缘层而不是 Linux 端（断网自治 + 实时性）。

### 6.5 FreeRTOS 任务划分

| 任务 | 优先级 | 职责 |
|---|---|---|
| Rs485TransportTask | 高 | 独占 UART/RS485，执行事务 |
| PollingSchedulerTask | 中高 | 生成周期读取/写请求 |
| RegisterModelTask | 中 | 更新缓存、有效期、状态 |
| NetworkTask | 中 | LwIP 连接与数据上报（可选） |
| HealthMonitorTask | 低 | 任务心跳、栈余量、喂狗 |

通信关系：

```
PollingSchedulerTask
        ↓ RequestQueue
Rs485TransportTask
        ↓ ResultQueue
RegisterModelTask
        ↓ SnapshotQueue
NetworkTask
```

约束：

- 禁止多个任务直接访问 UART
- 只有所有关键任务健康时才喂狗；HealthMonitorTask 维护各任务心跳位图，全部在有效期内才喂
- 初始队列深度建议：RequestQueue 16、ResultQueue 16、SnapshotQueue 8；W3 用实测占用调整
- 记录 `uxTaskGetStackHighWaterMark()` 与 `uxQueueMessagesWaiting()`
- 栈大小、队列深度必须能解释，不要拍脑袋

---

### 6.6 F407 → Linux 应用层协议

固定头：

```c
#define GW_MAX_PAYLOAD 1024

#pragma pack(push, 1)
typedef struct {
    uint16_t magic;           // 0xA55A
    uint8_t  version;         // 1
    uint8_t  message_type;
    uint32_t sequence;
    uint32_t gateway_id;
    uint32_t timestamp_ms;    // 网关单调 uptime 毫秒，不依赖 RTC
    uint16_t payload_length;  // <= GW_MAX_PAYLOAD
    uint16_t payload_crc;     // CRC16，覆盖 payload
    uint8_t  flags;           // 预留
    uint8_t  reserved[3];     // 预留，置 0
    uint16_t header_crc;      // CRC16，覆盖本字段之前的 24 字节
} GatewayMessageHeader;
#pragma pack(pop)
```

固定头 26 字节。

消息类型：

```c
typedef enum {
    MSG_HELLO = 0x01,
    MSG_HEARTBEAT = 0x02,
    MSG_DEVICE_DATA = 0x03,
    MSG_DEVICE_STATUS = 0x04,
    MSG_ALARM = 0x05,
    MSG_COMMAND = 0x10,
    MSG_COMMAND_RESPONSE = 0x11
} MessageType;
```

要求：

- 固定头 26 字节；`#pragma pack(1)`；所有多字节字段网络字节序（大端）
- `payload_length` 最大 1024 字节
- `timestamp_ms` 是网关单调 uptime 毫秒，不依赖 RTC；Linux 落库时间用收到报文的 wall clock
- `payload_crc` 覆盖 payload；`header_crc` 覆盖本字段之前的 24 字节
- CRC 统一用 CRC-16/CCITT-FALSE（0x1021，初值 0xFFFF），测试向量 `"123456789"` → 0x29B1；与 Modbus 的 0xA001 实现分开
- `sequence` 每个连接从 1 开始，用于丢包/重复/乱序检测，回绕按无符号处理
- 错误 magic、版本不支持、长度超限、CRC 失败一律断开连接并记录
- `MSG_DEVICE_DATA` 采用批量点位格式（一条消息多个点）；**全部 payload 格式已于 2026-11-26 冻结于 `docs/gateway-architecture.md` §3**
- TCP 是字节流，必须处理半包/粘包

---

### 6.7 Linux 服务

#### 技术选型

- **C**，不是 C++
- CMake
- 非阻塞 socket + epoll
- SQLite
- 可选 systemd（第二阶段）

#### 线程模型

```
Main Thread
├── epoll 事件循环
├── accepter
├── socket read/write
└── 心跳检查

Database Thread
└── SQLite 批量写入

Logger Thread
└── 异步日志
```

#### 必须处理

- 非阻塞 socket
- EPOLLIN / EPOLLOUT
- 对端关闭
- EINTR / EAGAIN
- TCP 半包
- TCP 多包粘连
- 每个连接独立接收缓存
- 心跳超时
- 发送缓冲区未全部发送

不能假设一次 `recv()` 就能拿到完整一帧。

#### SQLite 表

```sql
CREATE TABLE gateway (
    gateway_id INTEGER PRIMARY KEY,
    first_seen INTEGER,
    last_seen INTEGER,
    online INTEGER,
    firmware_version TEXT
);

CREATE TABLE device (
    gateway_id INTEGER,
    slave_id INTEGER,
    online INTEGER,
    last_seen INTEGER,
    offline_count INTEGER,
    PRIMARY KEY (gateway_id, slave_id)
);

CREATE TABLE device_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    gateway_id INTEGER,
    slave_id INTEGER,
    point_id INTEGER,
    register_space INTEGER,
    register_address INTEGER,
    value_raw INTEGER,
    scale REAL,
    quality INTEGER,
    timestamp INTEGER
);

CREATE INDEX idx_device_data_point_time
    ON device_data (gateway_id, slave_id, point_id, timestamp);

CREATE TABLE fault_event (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    gateway_id INTEGER,
    slave_id INTEGER,
    fault_code INTEGER,
    detail TEXT,
    timestamp INTEGER
);
```

- `timestamp` 统一为 Unix 毫秒（UTC）；`last_seen` 同单位
- `quality`：0 正常、1 过期、2 通信错误、3 越限
- 存原始 uint32 + scale，不要用 REAL 存所有值
- 一个逻辑点一行，不按 16 位寄存器拆行；32 位值由网关一次写入
- 不要每来一个寄存器就提交一次事务；每 500ms–1s 批量提交
- 开启 WAL
- 入库策略：变化触发 + 每个点位最小 1 秒间隔；50 个点位约 430 万行/天，含索引约 0.3–0.5GB/天
- 保留期默认 7 天，按天清理；保留期可配置
- 24 小时测试记录行数、数据库大小、WAL 大小

#### 验收

- PC 模拟多个网关客户端，服务稳定接收/解析/入库
- 24 小时无崩溃、无内存泄漏（ASan）
- 断线重连、服务重启后可恢复
- 数据库行数/大小符合入库策略，清理任务正常

---

### 6.8 Python 工具

- `modbus_simulator`：pymodbus 模拟 4 个从站，支持 0x03/0x04/0x06/0x10 和寄存器表
- `modbus_master_tester`：PC 主站测试脚本，W1 验收用（连续读 1000 次、异常码、写回显）
- `fault_injector`：按从站注入 CRC 错误、超时、异常码、截断帧、TCP 半包/粘包、断线
- `gateway_simulator`：按冻结协议生成合法 TCP 报文，W5 无 LwIP 时给 Linux 服务供数
- `log_analyzer`：统计成功率、P50/P95 延迟、故障次数
- `test_reports`：生成 Markdown/CSV 报告

---

## 7. 测试计划

### 7.1 Modbus

| 编号 | 场景 | 预期 |
|---|---|---|
| MB-01 | 0x03 正常读 | 返回正确寄存器 |
| MB-02 | 0x04 正常读 | 返回输入寄存器 |
| MB-03 | 0x06 写单寄存器 | 写入并回显 |
| MB-04 | 0x10 写多寄存器 | 批量写入成功 |
| MB-05 | CRC 错误 | 丢弃并计数 |
| MB-06 | 非法功能码 | 异常码 0x01 |
| MB-07 | 非法地址 | 异常码 0x02 |
| MB-08 | 非法数据值 | 异常码 0x03 |
| MB-09 | 响应超时 | 重试后失败 |
| MB-10 | 连续失败 | 离线降频 |
| MB-11 | 设备恢复 | 连续成功后上线 |
| MB-12 | 广播写 0x06 | 从站执行、不响应 |
| MB-13 | 0x10 数量超限 | 异常码 0x03 |
| MB-14 | 0x03 读只读输入区地址 | 异常码 0x02 |
| MB-15 | 故障注入超时 | 30 秒后自动恢复 |

### 7.2 TCP

- 一帧拆多次发送
- 多帧合并一次发送
- 发送一半断开
- 重复序号
- 错误 Magic
- header CRC 错
- payload CRC 错
- Payload 长度超限
- 网关断开重连
- Linux 服务重启
- SQLite 暂时写入失败
- 心跳超时

### 7.3 稳定性

第一阶段：

- 4 个逻辑从站
- 50 个采集点（F103 实表 18 点，压力测试由模拟器补齐）
- 最短轮询周期 200ms
- RS485 115200 8N1
- 连续 24 小时
- 无死锁、无看门狗复位
- ASan 无内存错误
- 单设备离线不影响其他设备
- 入库按“变化触发 + 1s 下限”，数据库行数/大小符合预期

第二阶段：

- 8 个逻辑从站
- 100–200 个采集点
- 72 小时
- 30+ 故障场景

---

## 8. 6 周计划（配合 CET-4）

> 12/12 前每天最多 1 小时，只做文档、寄存器表、CRC16；12/13 后加速。

| 周 | 目标 | 验收 |
|---|---|---|
| W1 | F103 从站（4 个 Slave ID）0x03/0x04/0x06/0x10 + CRC16 + RS485 + 故障注入 | 0x03/0x04 各连续读 1000 次无错，写回显正确，非法地址返回 0x02 |
| W2 | F407 主站 0x03/0x04/0x06/0x10 + 超时/重试 | 单从站读写正常，拔线超时 |
| W3 | FreeRTOS 非阻塞状态机 + 队列 | 通信期间其他任务正常运行 |
| W4 | 4 从站调度 + 寄存器镜像 + 离线/恢复 | 关闭一个从站不影响其他；形成 MCU MVP |
| W5 | Linux C epoll + SQLite + gateway_simulator | 模拟多网关，数据按策略入库 |
| W6 | LwIP + 联调 + 故障注入 + 24h + README | 20 个场景 + 演示视频 |

> W1 提前做 4 个功能码，是为了避免 W2 主站测试 0x04/0x10 时从站不支持而返工。

**关键节点：第 4 周结束就能写简历，不要等全部做完。**

---

## 9. AI 协作规则

### 9.1 必须遵守

1. **一次只做一个模块**，不生成整个项目
2. 先给接口、状态机、错误码、测试用例，再给代码
3. 代码用 C99，无动态内存，固定缓冲区，静态分配
4. F407 FreeRTOS 任务中禁止 `HAL_Delay()`；用 `osDelay`/定时器。F103 裸机只允许初始化阶段短延时，主循环用 SysTick 非阻塞
5. 中断共享变量加 `volatile`；临界区保护
6. CRC/解析器必须提供测试向量
7. 关键逻辑要解释“为什么”，不只给代码
8. 硬件引脚/UART 实例/时钟不明确时，先问用户
9. 不要写 LAN8720；F407 是 YT8512C
10. 不要编造实测数字
11. RS485 固定 115200 8N1；测量值走 0x04，参数/控制走 0x03；地址一律 0-based
12. 协议头、寄存器表、镜像映射是冻结项；改动必须同步 F103/F407/Python/Linux 四处，并更新 docs
13. 单次 Modbus 读取不超过 20 个寄存器；超时默认 50ms，改动要重新核算帧时间

### 9.2 代码风格

- 模块化：`modbus/`、`scheduler/`、`register_model/`、`storage/`、`telemetry/`、`health_monitor/`
- 头文件 + 源文件，接口清晰
- 注释用中文，解释状态/错误码/时序
- 错误码统一枚举
- 不用全局变量堆逻辑；用上下文结构体
- 每个模块可单独测试
- 寄存器表以 `docs/register-map.md` 为唯一来源；改表必须同步 F103 表、F407 映射、Python 模拟器和文档

### 9.3 每次回答的格式

1. 这个模块的输入/输出/状态/错误码
2. 设计思路与关键时序
3. 代码（只给该模块）
4. 测试用例/测试向量
5. 风险与需要用户确认的硬件信息

### 9.4 红区（用户必须自己写/必须能解释）

- RS485 DE/RE 时序
- 非阻塞事务状态机
- 多从站调度器
- 寄存器镜像有效期/权限/字节序
- 应用层协议头、CRC 与 TCP 拆包
- 双区配置提交顺序
- FreeRTOS 任务/队列/心跳/喂狗
- 硬件联调与 bug 定位

AI 可以 review、补全、解释，但不能替用户“拥有”这些模块。

---

## 10. 交付物

```
仓库根目录（当前 /workspace，不再嵌套一层）
├── README.md
├── docs/
│   ├── architecture.md
│   ├── protocol.md
│   ├── register-map.md
│   ├── test-plan.md
│   └── images/
├── gateway-f407/
│   ├── Core/
│   ├── Drivers/
│   ├── App/
│   │   ├── modbus/
│   │   ├── scheduler/
│   │   ├── register_model/
│   │   ├── storage/
│   │   ├── telemetry/
│   │   └── health_monitor/
│   └── README.md
├── device-f103/
│   ├── Core/
│   ├── Drivers/
│   ├── App/
│   │   ├── modbus_slave/
│   │   ├── register_map/
│   │   └── fault_injection/
│   └── README.md
├── linux-server/
│   ├── CMakeLists.txt
│   ├── include/
│   ├── src/
│   ├── config/
│   ├── scripts/
│   ├── tests/
│   └── README.md
└── tools/
    ├── modbus_simulator/
    ├── modbus_master_tester/
    ├── gateway_simulator/
    ├── fault_injector/
    ├── log_analyzer/
    └── test_reports/
```

最终要有：

- GitHub 仓库可复现
- README：架构图、寄存器表、协议、测试结果、演示视频
- 2 分钟演示视频
- 测试报告（24/72h、故障场景、P50/P95、栈余量、队列占用、数据库行数/大小）

---

## 11. 简历写入规则

只在对应阶段完成后写：

**第 4 周后（MCU MVP）**

> 基于 STM32F407、FreeRTOS 与 RS485 实现 Modbus RTU 主站，设计非阻塞事务状态机和多从站调度器，支持超时重试、离线降频、恢复上线及寄存器镜像。

不要写 Linux。

**第 6 周后（Linux 接入）**

> 基于 LwIP 与 Linux C 建立 TCP 数据通路，Linux 端使用 epoll 处理连接与消息拆包，并通过 SQLite 持久化设备数据。

**测试完成后**

> 使用 Python 执行 [X] 类故障注入测试，系统管理 [N] 个从站、[M] 个采集点，连续运行 [XX] 小时。

**禁止：**

- 未完成就写
- 未实测的数字
- 把“计划”写成“已实现”
- 写 LAN8720（实际是 YT8512C）

---

## 12. 已知风险与应对

| 风险 | 应对 |
|---|---|
| LwIP/YT8512C 调不通 | 放第二阶段；先用 PC 模拟器/串口桥，不阻塞 MCU MVP |
| RS485 时序/干扰 | 逻辑分析仪看 DE/RE；120Ω 终端；双绞线；共地 |
| FreeRTOS 栈溢出 | 记录 high water mark；先给足栈，再逐步收紧 |
| Modbus CRC 错 | 用已知测试向量；先 PC 模拟器验证 |
| TCP 粘包 | 每连接独立缓存；先解析头再按长度取帧 |
| SQLite 写放大 | 批量提交 + WAL；入库按变化触发 + 1s 下限 + 7 天保留 |
| 115200 判帧不稳 | DMA + IDLE；逻辑分析仪确认；异常时回退 19200 + 500ms 周期 |
| 24h 数据量过大 | 变化触发 + 1s 下限 + 保留期；测试前估算行数 |
| 协议/寄存器表改动扩散 | 冻结 v1；register-map.md 为唯一来源，改表同步四处 |
| 时间不够 | 第 4 周先形成 MCU MVP，Linux/LwIP 作为加分 |

---

## 13. 第一周任务（只做这个）

**目标：F103 Modbus RTU 从站（4 个逻辑 Slave ID）0x03 最小闭环，同步完成 0x04/0x06/0x10。**

开始写代码前先产出：

1. `docs/register-map.md`：4 个从站的 0x03/0x04 表、缩放、镜像映射、故障注入寄存器
2. `device-f103/App/modbus_slave/` 的文件结构与接口
3. 帧解析状态机（等待地址 → 功能码 → 长度/数量校验 → 地址/空间校验 → CRC → 响应）
4. CRC16 测试向量（`"123456789"` → 0x4B37）与 0x03/0x04/0x06/0x10 测试用例
5. 异常码 0x02/0x03 与广播写测试用例
6. 需要用户确认的硬件信息（F103 型号、UART 实例、DE/RE 引脚、RS485 模块是否自动收发）

**本周不要做：** FreeRTOS、LwIP、Linux、SQLite、双区配置。

**验收：** PC 发合法 0x03/0x04 返回正确寄存器；0x06/0x10 写入并回显；非法地址返回 0x02；115200 下连续读 1000 次无错。

---

*文档版本：v1.1 · 2026-11-24 · 冻结 RS485 115200、4 从站、0x03/0x04 划分、26 字节协议头、入库策略*
