# 任务单 W2–W6（可直接交给执行模型）

> 架构师交付物（2026-11-26）。W1 任务单见 `w1-execution-plan.md`。
> 每张单限制允许/禁止修改文件；执行模型只改"允许"列；红区（**加粗负责人=你**）不得代写。
> 通用禁止：HAL 库源码、启动文件、系统时钟配置、`docs/AI-HANDOFF.md`、`docs/register-map.csv`、`docs/gateway-architecture.md`、他端模块文件。
> 通用 DoD：编译 0 warning（-Wall）、主机测试过、你 review、实测数字入档。

---

## W2-T1 F407 工程与板级配置（你；AI 给配置清单）
- 允许：`gateway-f407/Core/*`（CubeMX 生成）、`gateway-f407/App/port/board_config_f407.h`
- 输入：`hardware-f407-v3.md`（USART2 PA2/PA3、PG8 高=发送、P4 跳线 RS485 侧）
- 输出：可编译工程；USART1 打印 hello
- 边界：PG8 与 NRF IRQ 共用→不插 NRF；时钟 HSE 8M→168M 按板卡事实
- 验收：hello 可见；编译通过

## W2-T2 F407 RS485 port + 事务状态机（**你，红区**）
- 允许：`gateway-f407/App/port/rs485_port.c/.h`、`gateway-f407/App/modbus_master/mb_master.c/.h`
- 输入：`rs485-port-spec.md`、`gateway-architecture.md` §1.3/§1.4、冻结 §6.2 状态枚举
- 输出：整帧收发（DMA+IDLE、TC 后拉低 DE）；9 态事务状态机
- 必须处理：DMA 完成≠发完；响应超时；CRC 错；异常响应不重试；帧长越界；从站地址不符
- 验收：单从站 0x03/0x04/0x06/0x10 全通；逻辑分析仪见 DE 在 TC 后释放

## W2-T3 超时/重试/离线计数（**你，红区**）
- 允许：`mb_master.c`、`gateway-f407/App/stats/mb_stats.c/.h`
- 输出：retry_max=2 队尾重试；连续 3 失败离线；计数结构
- 边界：重试不原地阻塞；重试期间其他从站请求可插入
- 验收：拔线→超时/重试/失败计数与脚本断言一致

## W2-T4 主站侧测试脚本（执行模型）
- 允许：`tools/modbus_master_tester/*`、`tests/host/test_master_*.py`
- 输出：拔线/坏帧/异常码场景脚本 + 断言
- 验收：脚本对 W2-T2/T3 实测日志全过

## W3-T1 FreeRTOS 静态任务/队列骨架（**你，红区**）
- 允许：`gateway-f407/App/rtos/app_tasks.c/.h`、`app_queues.c/.h`
- 输入：`gateway-architecture.md` §1（优先级/栈/队列/满策略/同步原语）
- 输出：5 任务静态创建；Request/Result/Frame/Snapshot/Write 队列；单所有者 UART
- 边界：无 malloc；ISR 只入队/通知；每个等待点有 timeout
- 验收：通信期间 LED/日志任务正常运行；队列无静默溢出

## W3-T2 健康监控/喂狗/水位统计（你设计；执行模型写）
- 允许：`gateway-f407/App/health/health_monitor.c/.h`
- 输出：心跳 event group；IWDG 2s 全新鲜才喂；HWM/队列峰值周期记录
- 验收：故意卡死一个任务→2s 内复位且原因入统计

## W3-T3 并发压测脚本（执行模型）
- 允许：`tests/host/test_rtos_*.py`、`tools/log_analyzer/*`
- 验收：压测日志含栈水位/队列峰值表，供 W3 门禁引用

## W4-T1 多从站调度器（**你，红区**）
- 允许：`gateway-f407/App/scheduler/poll_scheduler.c/.h`
- 输入：冻结 §6.3 PollItem 与调度表（500/1000/200/1000ms，离线 5/10/2/5s）
- 必须处理：写优先；队尾重试；离线降频；恢复 2 次成功；队列满计数
- 验收：关 Slave 2 不影响 1/3/4；恢复自动上线

## W4-T2 寄存器镜像（**你，红区**）
- 允许：`gateway-f407/App/register_model/register_model.c/.h`
- 输入：冻结 §6.4 RegisterEntry；`gateway-architecture.md` §1.2 单写者+快照池
- 必须处理：32 位一次性更新；有效期过期标 quality=1；控制点回显确认后才 valid
- 验收：快照无半新半旧（测试脚本断言 32 位点原子性）

## W4-T3 边缘控制闭环（**你**）
- 允许：`gateway-f407/App/edge_control/edge_control.c/.h`
- 输入：冻结 §6.4 闭环规则（≥300 开、≤280 关，回差 2°C）
- 边界：Slave 3 离线或温度 invalid 不动作；状态变化才写
- 验收：升温自动开、回落自动关；阈值附近不抖动（脚本扫 280–300 边界）

## W4-T4 调度/镜像/闭环场景脚本（执行模型）
- 允许：`tests/host/test_scheduler_*.py`、`test_mirror_*.py`、`test_edge_*.py`
- 验收：G3 门禁全部断言过

## W5-T1 Linux epoll 服务（执行模型 R；你 A）
- 允许：`linux-server/src/*`、`linux-server/CMakeLists.txt`
- 输入：`gateway-architecture.md` §3（26B 头大端、payload 格式、环 2048B、8 槽）
- 必须处理：EINTR/EAGAIN、半包/粘包、对端关闭、心跳超时、发送未写完
- 验收：gateway_simulator 多连接并发无丢帧解析；断线重连恢复

## W5-T2 SQLite 持久化（执行模型 R；你 A）
- 允许：`linux-server/src/db/*`
- 输入：冻结 §6.7 表结构；变化触发+1s 下限+保留期；WAL；500ms–1s 批量
- 验收：入库行数/天实测；保留期清理生效；quality 列正确

## W5-T3 网关模拟器+协议一致性测试（执行模型）
- 允许：`tools/gateway_simulator/*`、`tests/host/test_protocol_*.py`
- 必须覆盖：半包、粘包、坏 magic、坏双 CRC、sequence gap、version 错、超长
- 验收：一致性测试全过（这是三端唯一版本的执法测试）

## W5-T4 独立审查（审查模型；不改代码）
- 输入：W5 头文件+关键 .c+日志；输出：死锁/越界/竞态/协议边界清单
- 验收：清单逐条闭环后才过 G4

## W6-T1 LwIP 接入（可选；你拍板；执行模型骨架）
- 允许：`gateway-f407/App/net/*`
- 边界：卡住 2 天即砍，回退串口桥/模拟器；不阻塞 G5
## W6-T2 24h + 20 故障场景（你跑；执行模型脚本）
- 允许：`tests/stability/*`
- 验收：24h 无复位、20 场景通过；数字入档
## W6-T3 报告/README/简历指标（执行模型初稿；你批准）
- 允许：`docs/test-report-w6.md`、展示仓库 README 草稿（本地）
- 边界：只填实测数字；未验收模块不写
