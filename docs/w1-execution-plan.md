# W1 执行计划（无板日 + 到手日）

> 2026-11-25 更新：F103（江协套件）与探索者 V3 **已在手**，MAX3485 模块待购。
> 因此优先执行 §6「TTL 直连方案」——不装 RS485 模块也能先跑通协议层；模块到货后再补 §2 的 M1/M6。
>
> 目标：板子到手后，一天内跑通"PC 主站 ↔ F103 从站"最小闭环。
> 原则：今天做不依赖硬件的事；明天只做必须上板的事。
> 时间盒：今天 ≤1.5h，明天 ≤4h；CET-4 复习不挪。

## 1. 今天（板子不在身边）

### U1. 建 CubeMX 工程（30 分钟）

- MCU：STM32F103C8T6；
- 时钟：72MHz；
- USART2：115200、8N1、开启全局中断；
- USART2 RX/TX 各开 DMA（F103：RX=DMA1_Channel6，TX=DMA1_Channel7）；
- 开启 USART2 的 IDLE 中断；
- USART1：115200，调试串口；
- PA8：推挽输出，默认低电平（接收）；
- 生成 Keil 或 STM32CubeIDE 工程，放在 `device-f103/`。

### U2. 加源码、先编译（45 分钟）

Include 路径：

```text
third_party/nanomodbus
shared/registers
device-f103/App
device-f103/App/port
device-f103/App/register_map
device-f103/App/modbus_slave
device-f103/App/fault_injection
```

源文件：

```text
shared/registers/regs_gen.c
third_party/nanomodbus/nanomodbus.c
device-f103/App/app_f103.c
device-f103/App/register_map/mb_regs.c
device-f103/App/modbus_slave/mb_slave.c
device-f103/App/modbus_slave/mb_router.c
device-f103/App/fault_injection/fault_injection.c
```

先不写 `rs485_port.c` 也能编译到链接前；把编译错误逐条修掉。
注意：`board_config.h` 里是占位引脚，编译能过，但上板前必须核对。

### U3. 读规格、列 rs485_port.c 的函数清单（15 分钟）

读 `docs/rs485-port-spec.md`，在纸上写出这 6 个函数的行为：

1. `rs485_port_init`
2. `rs485_port_irq_handler`
3. `rs485_port_write`
4. `rs485_port_read_frame`
5. `rs485_port_tick`
6. `rs485_port_stats`

重点写清：DE 什么时候拉高、什么时候拉低、TC 在哪里等。

### U4. 核对板子引脚（10 分钟）

明天带板子前，先找 C8T6 最小系统板原理图或丝印，确认：

- USART2 是否是 PA2/PA3；
- PA8 是否方便接 DE；
- USART1 PA9/PA10 是否引出；
- 3.3V、GND 是否方便共地。

不一致就只改 `board_config.h`，不要改时序。

## 2. 明天（板子到手）

### M1. 接线与上电（30 分钟）

- C8T6 3.3V、GND 接 MAX3485 模块；
- PA2 → DI，PA3 → RO，PA8 → DE/RE；
- A/B 接 USB-RS485，A/B 不要接反；
- 总线两端 120Ω（桌面短线可先一端）；
- 共地；
- 上电前万用表量 3.3V 与 GND 不短路。

### M2. 烧录与调试串口（30 分钟）

- 烧一个最小程序，USART1 打印 `hello`；
- 确认波特率 115200、接线正确；
- 再烧完整工程，确认不 HardFault。

### M3. PC 主站读第一个寄存器（60 分钟）

- 运行 `tools/modbus_master_tester/master_tester.py --port COMx --slave 1 --function 4 --address 0 --count 4 --iterations 1`；
- 能看到响应即通过；
- 不通时按顺序查：DE 电平、A/B、波特率、从站地址、CRC。

### M4. 四个从站与功能码（60 分钟）

- Slave 1–4 各读一次 0x03/0x04；
- 0x06 写单寄存器并回显；
- 0x10 写多寄存器；
- 非法地址返回 0x02，非法功能码返回 0x01；
- 广播写 0x06：执行但不响应。

### M5. 1000 次连续读（60 分钟）

```bash
python master_tester.py --port COMx --slave 1 --function 4 --address 0 --count 4 --iterations 1000
```

记录：成功/失败、min/avg/p95/max。
失败时先看是不是 DE 释放太早、DMA 重启丢帧、队列溢出。

### M6. 故障注入与波形（60 分钟）

- 写 `0x00F0 = 1`（NO_RESPONSE），确认 30s 后自动恢复；
- 写 `0x00F0 = 2`（BAD_CRC），确认主站报 CRC 错；
- 写 `0x00F0 = 3`（DELAY），确认主站超时/重试；
- 逻辑分析仪看：DE 拉高 → 数据 → TC → DE 拉低；
- 截图保存到 `docs/evidence/w1/`。

### M7. 记录证据（30 分钟）

- 测试命令、commit SHA、接线照片、波形截图、日志；
- 填 `docs/w1-status.md` 的"已完成"；
- 结论只写实测，不写"应该"。

## 3. 谁做什么

| 任务 | 你 | AI |
|---|---|---|
| CubeMX 工程 | 做 | 给配置清单 |
| 编译修错 | 做 | 看报错给假设 |
| `rs485_port.c` | **写** | review，不代写 |
| `main.c` 接线 | **写** | 给 snippet |
| 硬件接线/波形 | **做** | 看波形给排查方向 |
| 1000 次测试 | **跑** | 分析日志 |
| `mb_regs.c` 权限 | 审查 | 已给实现 |
| `fault_injection.c` | 审查 | 已给实现 |
| 测试报告 | 提供数据 | 整理初稿 |

红区不外包：DE/RE 时序、UART IDLE 判帧、主循环集成、硬件定位。

## 4. 验收清单

- [ ] USART1 能打印日志
- [ ] Slave 1 的 0x04 能读到 4 个寄存器
- [ ] Slave 1–4 都能响应
- [ ] 0x06/0x10 写入并回显
- [ ] 非法地址返回 0x02
- [ ] 广播写执行但不响应
- [ ] 1000 次读 0 失败
- [ ] NO_RESPONSE 30s 自恢复
- [ ] 逻辑分析仪看到 DE 在 TC 后释放
- [ ] 测试记录进 `docs/evidence/w1/`

## 5. 卡住时的降级

- 115200 不稳 → 先降 19200 + 500ms 周期，跑通再升；
- DE 控制不对 → 先用自动收发模块验证协议，再回来调 GPIO；
- C8T6 串口不够 → 先用 USART1 做 RS485，USART2 做调试；
- 板子当天没调通 → 不硬撑，记录现象，第二天只改一个变量。

## 6. TTL 直连方案（等 RS485 模块期间的先行路径）

**原理**：Modbus RTU 只规定 UART 帧格式，RS485 只是物理层。USB-TTL 直连 F103 的 USART2，可以完成除电气层以外的全部协议验证。

### 6.1 接线（3 根线，交叉）

| USB-TTL | F103 | 说明 |
|---|---|---|
| TXD | PA3（USART2_RX） | 交叉 |
| RXD | PA2（USART2_TX） | 交叉 |
| GND | GND | 必须共地 |
| 电平跳线 | 选 **3.3V** | 不要 5V；VCC 不接，板子自己供电 |

- PA8 照常配置为输出并写 DE 时序代码，模块未到时空脚，不影响运行；
- **模块到货后拔掉 USB-TTL 的 TX/RX**，否则会和 RS485 收发器抢 PA2/PA3。

### 6.2 能验证 / 不能验证

| 可提前通过 | 必须等模块 |
|---|---|
| 4 个从站 0x03/0x04 读 | DE/RE 时序与 TC 等待 |
| 0x06/0x10 写与回显 | 半双工总线冲突、A/B 极性 |
| 非法地址 0x02、非法功能码 0x01 | 120Ω 终端与差分波形 |
| 广播写执行不响应 | 115200 长线稳定性 |
| 1000 次连读、延迟统计 | 逻辑分析仪波形截图 |
| 故障注入（NO_RESPONSE/BAD_CRC/EXCEPTION/TRUNCATED） | DELAY 需配合主站超时观察 |

### 6.3 命令

```bash
python tools/modbus_master_tester/master_tester.py --port COMx --slave 1 --function 4 --address 0 --count 4 --iterations 1000
```

工具已支持 `--port` 串口模式，无需改代码。

### 6.4 上板顺序（有板即可做）

1. CubeMX 建工程（§1 U1）→ 编译（§1 U2）→ 烧录 USART1 打印 hello；
2. 写 `rs485_port.c` 第一版：DMA + IDLE 收帧、发送等 TC（DE 代码先写、暂无模块）；
3. USB-TTL 接线，跑单次读 → 4 从站 → 写回显 → 1000 次；
4. 模块到货：改接线为 A/B 总线，补做 M6 波形与 DE 时序验证。

### 6.5 江协套件的额外用途

- 套件里的电位器接 PA0（ADC1_IN0）做分压实验，为 Slave 2 真实电压采集打底；
- LED/按键可做 USART 中断与主循环节拍的观察点；
- 若套件带 DAP-Link，可直接烧录；否则用已有的 ST-Link（SWD：PA13/PA14、GND、3.3V）。
