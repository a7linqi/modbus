# Industrial Edge Gateway

基于 FreeRTOS 与 Linux 的工业边缘通信系统：

**STM32F103（Modbus RTU 从站）→ RS485 → STM32F407（Modbus RTU 主站/网关）→ 以太网 → Linux C 服务（epoll + SQLite）**

## 项目状态

- 状态：未开工
- 第一里程碑：F103 Modbus RTU 从站 0x03 最小闭环
- 目标：6 周形成可写简历的 MCU MVP，再接入 Linux/LwIP

## 文档

- [AI 交接文档](docs/AI-HANDOFF.md)：项目范围、架构、协议、测试计划、AI 协作规则
- 架构图、寄存器表、协议说明、测试报告将在开发过程中补充

## 硬件

| 硬件 | 说明 |
|---|---|
| STM32F407 探索者 V3 | 板载 YT8512C 以太网 PHY、SP3485 RS485、TJA1050 CAN |
| STM32F103 开发板 | Modbus RTU 从站 |
| USB-RS485 | PC 调试 / 从站模拟 |
| 120Ω 电阻 | RS485 总线终端 |

## 目录规划

```
industrial-edge-gateway/
├── docs/            # 架构、协议、寄存器表、测试计划
├── gateway-f407/    # FreeRTOS 网关固件
├── device-f103/     # Modbus RTU 从站固件
├── linux-server/    # Linux C epoll 服务
└── tools/           # Python 模拟器、故障注入、日志分析
```

## 说明

- 本项目为个人学习/求职项目，当前以 MVP 为主，不追求一次做完所有功能。
- 未完成的模块不会写入简历；所有指标以实测为准。
