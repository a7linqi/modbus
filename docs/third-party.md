# 开源选型与复用边界

> 原则：协议栈和测试工具不重造轮子；时序、调度、镜像、故障注入这些"能讲出深度"的部分自己写。

## 1. 候选清单（已核查）

| 库 | 语言 | 许可 | 主站 | 从站 | RTU | TCP | 适合 |
|---|---|---|---|---|---|---|---|
| FreeMODBUS 官方核心（cwalter-at/freemodbus） | C | 核心 BSD；移植层 LGPL；demo GPL | 官方开源版不含 | 成熟 | 是 | 是 | 从站参考，别抄 demo/port |
| armink/FreeModbus_Slave-Master-RTT-STM32 | C | README 声明 BSD（GitHub 标 Other，需看文件头） | 有，eMBMasterPoll | 有 | 是 | 否 | 主从一体底座，但 OS 适配是 RT-Thread |
| nanoMODBUS（debevv/nanoMODBUS） | C99 | MIT | 有，同步 client | 有，回调式 server | 是 | 是 | 最干净，单文件约 2000 行，无动态内存 |
| pymodbus | Python | BSD-3-Clause | 有 | 有 | 有 | 有 | PC 主站/从站模拟/故障注入 |
| libmodbus（stephane/libmodbus） | C | LGPL-2.1+ | 有 | 有 | 有 | 有 | Linux 二期；不替代自定义 26 字节协议 |

注意：FreeMODBUS 官方开源版只有从站，主站是商业功能；armink 版本补了主站，但 Star 高、最后提交较旧，移植要自己验证。

## 2. 推荐方案

### 方案 A（推荐）：armink FreeMODBUS 主从扩展 + pymodbus

- 只取 `modbus/` 协议核心，自己写 FreeRTOS port（事件、定时器、临界区）和 DMA/IDLE 串口移植。
- F103 用 `eMBPoll`，F407 用 `eMBMasterPoll`，保留事件驱动模型，符合非阻塞要求。
- 调度、离线/恢复、镜像、故障注入全部自研。
- 风险：RT-Thread 移植层要改；老代码需要 review；许可要逐文件看头注释。

### 方案 B（最干净、最快跑通 W1）：nanoMODBUS + pymodbus

- F103 从站直接用 nanoMODBUS server，只实现 `read/write` 两个平台函数。
- F407 主站 client 是同步调用，需要包在独立任务里，或改用自研状态机 + 库的编解码。
- 适合先把 W1 从站验收跑通；W2 再决定主站是否切到方案 A。

### 不建议

- 从零写 Modbus 协议栈：浪费时间，且协议正确性不如成熟库。
- 用 libmodbus 上 MCU：面向 POSIX，不合适。
- 直接复制网上"STM32 移植 FreeMODBUS"工程：常带 GPL demo/LGPL port，且时序未必对。

## 3. 自研保留区（简历核心）

1. UART/DMA/IDLE 判帧 + DE/RE 等 UART TC 再释放。
2. 非阻塞事务状态机 + 总线周转（3.5 字符静默）。
3. 多从站调度、超时重试、离线降频、恢复上线。
4. 寄存器镜像：有效期、32 位原子更新、权限。
5. 寄存器表单一来源（CSV 生成 C/Python 常量）。
6. 故障注入与实测报告（基于 pymodbus 扩展）。

## 4. 引入方式

- 第三方代码放 `third_party/<name>/`，附 LICENSE 与来源 commit。
- 不直接改库源码；适配代码放 `port/` 或 `App/`。
- 展示仓库只保留必要第三方文件与许可说明，不塞整个历史。
- 每个库先跑官方测试/示例，再接硬件。

## 5. 对 W1 任务的影响

- 原"CRC16 从零写"改为：复用库实现，只写测试向量验证。
- 原"帧解析状态机从零写"改为：先接库，再补异常码、广播、故障注入。
- 你省下的时间投入到：DMA/IDLE 时序、DE/RE、1000 次连续读、故障注入与报告。

## 6. 完整项目参考（只学结构，不抄代码）

| 项目 | 规模 | 许可 | 能参考什么 | 不能做什么 |
|---|---|---|---|---|
| [1430038895/STM32F407-Modbus-RTU-TCP-gateway](https://github.com/1430038895/STM32F407-Modbus-RTU-TCP-gateway-FreeRTOS-LwIP-) | F407 + FreeRTOS + LwIP，8 从站 | README 标 MIT | 点表/寄存器镜像、轮询合并段、掉线分级退避、pymodbus 模拟器、任务/锁划分 | 不能直接抄代码；它用 LAN8720，你的板子是 YT8512C |
| [MoneShadow/stm32-industrial-protocol-gateway](https://github.com/MoneShadow/stm32-industrial-protocol-gateway) | F407 + F103 + CAN + Modbus Slave | 未声明（默认保留） | Device Model 设计、RS485 DMA+IDLE、CAN 与 Modbus 的职责边界、测试清单 | 不能复制代码；它是 Modbus 从站，不是主站 |
| [alejoseb/Modbus-STM32-HAL-FreeRTOS](https://github.com/alejoseb/Modbus-STM32-HAL-FreeRTOS) | 856 stars，主从 + RTU/TCP + DMA | LGPL-2.1 | 目录组织、FreeRTOS 任务封装、DMA 例子、pymodbus 测试脚本 | 不要静态链接进闭源/公开项目；LGPL 合规麻烦 |
| [eziya/STM32_HAL_FREEMODBUS_RTU](https://github.com/eziya/STM32_HAL_FREEMODBUS_RTU) | FreeMODBUS HAL 移植 | 未声明 | T3.5 定时器、UART 错误处理、事件队列 | 不能复制代码，只能读思路 |

**结论**：项目级参考用来"少走弯路"，不是"少写代码"。最值得学的是 1430038895 的**点表/镜像/退避**，以及 MoneShadow 的**Device Model 与测试清单**。

## 7. 参考映射到本项目

| 本项目模块 | 参考对象 | 借用方式 |
|---|---|---|
| 协议编解码/CRC | nanoMODBUS | 直接依赖（MIT） |
| F103 从站移植 | eziya / FreeMODBUS 官方 | 看 T3.5 与 UART 错误处理，自己写 HAL 适配 |
| F407 主站调度 | 1430038895 | 看点表/轮询合并/退避策略，自己写状态机 |
| 寄存器镜像 | 1430038895 / MoneShadow | 借数据结构思路，自己写有效期与原子更新 |
| PC 测试工具 | pymodbus | 直接依赖（BSD） |
| Linux 二期 | libmodbus | 只做 Modbus TCP 时可用（LGPL）；自定义协议仍自研 |
