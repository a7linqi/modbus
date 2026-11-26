# Industrial Edge Gateway

基于 FreeRTOS 与 Linux 的工业边缘通信系统：

**STM32F103（Modbus RTU 从站）→ RS485 → STM32F407（Modbus RTU 主站/网关）→ 以太网 → Linux C 服务（epoll + SQLite）**

## 项目状态

- 状态：W1 开发中（从站框架与寄存器模型已搭好，待硬件联调）
- 第一里程碑：F103 Modbus RTU 从站 0x03 最小闭环（W1 同步完成 0x04/0x06/0x10）
- 关键约定：RS485 固定 115200 8N1；4 个逻辑从站（3 业务 + 1 预留）；测量值走 0x04，参数/控制走 0x03
- 目标：6 周形成可写简历的 MCU MVP，再接入 Linux/LwIP

## 文档

- [项目总体框架](docs/project-framework.md)：系统组成、模块职责、数据流、里程碑
- [开发框架](docs/development-framework.md)：任务卡、分支、质量门禁
- [学习框架](docs/learning-framework.md)：每周学习地图与脱稿问题
- [人机分工](docs/agent-workflow.md)：谁写哪部分、红区、AI 边界
- [总览图](docs/diagrams.md)：全链路数据流 + 6 周排期（Mermaid）
- [准备期清单](docs/w1-status.md)：硬件 3 问、准备期状态、W1 移交
- [F407 板卡事实](docs/hardware-f407-v3.md)：探索者 V3 引脚、RS485 方向脚、时钟配置（带证据等级）
- [P3 Linux 扩展计划](docs/linux-edge-service-plan.md)：3 周自研 Linux 边缘服务（当前主线）
- [AI 交接文档](docs/AI-HANDOFF.md)：冻结约定与详细需求（工业网关线，已暂停）
- [网关架构设计](docs/gateway-architecture.md)：F407 任务/栈/队列/ISR 边界、三端一致性契约、里程碑门禁
- [任务单 W2–W6](docs/task-tickets.md)：允许/禁止修改文件、边界条件、验收定义

## 硬件

| 硬件 | 说明 |
|---|---|
| STM32F407 探索者 V3 | 板载 YT8512C 以太网 PHY、TPT8485 RS485（USART2 + PG8 方向）、TJA1050 CAN |
| STM32F103C8T6 最小系统板 | Modbus RTU 从站（64KB Flash / 20KB RAM） |
| MAX3485/SP3485 3.3V 模块 | F103 侧 RS485 收发器，DE/RE 由 GPIO 控制 |
| USB-RS485 | PC 调试 / 从站模拟 |
| 电位器 | PA0 ADC → Slave 2 电压真实采集 |
| AHT20 温湿度模块 | I2C1 PB6/PB7 → Slave 1 温度真实采集（压力点保留模拟源） |
| 120Ω 电阻 | RS485 总线终端 |

> 注意：探索者 V3 是 F407 板，不含 F103；F103 从站需要单独一块 C8T6 最小系统板。

## 目录规划

```
仓库根目录
├── docs/                 # 框架、协议、寄存器表、测试计划
├── shared/registers/     # 寄存器表生成物（F103/F407 共用）
├── device-f103/          # Modbus RTU 从站固件
├── gateway-f407/         # FreeRTOS 网关固件
├── linux-server/         # Linux C epoll 服务
├── third_party/          # 第三方库与许可证
├── tests/host/           # 主机测试
└── tools/                # 生成器、Python 主站/模拟器/故障注入
```

## 说明

- 本项目为个人学习/求职项目，当前以 MVP 为主，不追求一次做完所有功能。
- 未完成的模块不会写入简历；所有指标以实测为准。
