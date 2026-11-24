# W1 进度与分工清单

> 更新：2026-11-24。代码在 `trae/agent-l0JZZG` 分支，未提交/未推送。

## 已完成（AI/架构侧）

| 项 | 文件 | 验证 |
|---|---|---|
| 协议栈引入 | `third_party/nanomodbus/`（MIT，commit `91d6782`） | 源码与 LICENSE 齐全 |
| 寄存器表单一来源 | `docs/register-map.csv`（18 点） | 生成器校验通过 |
| 生成器 | `tools/register_map/gen_registers.py` | 生成 C/Python/Markdown |
| C 寄存器表 | `shared/registers/regs_gen.h/.c` | 地址/镜像区无重叠 |
| Python 寄存器表 | `tools/modbus_master_tester/regs_gen.py` | 可导入、lookup 正常 |
| CRC16 测试 | `tests/host/test_crc16.py` | 全部通过；标准 0x4B37，nanoMODBUS 0x374B |
| 主站测试工具 | `tools/modbus_master_tester/master_tester.py` | TCP 回环 5/5，平均 0.55ms |
| 寄存器模型 | `device-f103/App/register_map/mb_regs.h/.c` | 未编译（本机无 C 工具链） |
| 从站适配 | `device-f103/App/modbus_slave/mb_slave.h/.c` | 未编译；含异常响应与故障注入 |
| 4 从站路由 | `device-f103/App/modbus_slave/mb_router.h/.c` | 未编译；单播 + 广播写 + 轮询 |
| 故障注入 | `device-f103/App/fault_injection/fault_injection.h/.c` | 未编译；6 种模式，NO_RESPONSE 30s 自恢复 |
| 应用层 | `device-f103/App/app_f103.h/.c` | 未编译；模拟传感器 + 主循环 |
| RS485 接口 | `device-f103/App/port/rs485_port.h` | 仅接口 |
| RS485 实现规格 | `docs/rs485-port-spec.md` | 时序、错误处理、验收 |
| 板级配置模板 | `device-f103/App/port/board_config.h` | 未填写，故意 `#error` |

## 留给你的部分（必须你写/确认）

1. **填写 `board_config.h`**：F103 型号、USART 实例、TX/RX 引脚、DE/RE 引脚、是否自动收发。
2. **实现 `rs485_port.c`**：DMA + IDLE 判帧、DE/RE 等 UART TC、错误计数。这是红区，必须你写。
3. **把 `app_init()` / `app_poll()` 接到 CubeMX 生成的 `main.c`**：初始化后进主循环，`now_ms` 用 `HAL_GetTick()`。
4. **硬件联调**：先 USB-RS485 + PC 主站脚本，再接 F407。
5. **审查 `mb_regs.c` / `fault_injection.c`**：确认权限、清零命令、故障模式符合你的预期。

## 需要你回答的硬件信息

- F103 具体型号（C8T6？Flash/RAM）
- RS485 用哪个 USART，TX/RX 引脚
- DE/RE 接哪个 GPIO，高电平还是低电平发送
- RS485 模块是否自动收发
- 调试串口用哪个 USART

## 编译配置（Keil/CubeIDE）

- Include 路径：
  - `third_party/nanomodbus`
  - `shared/registers`
  - `device-f103/App`
  - `device-f103/App/port`
  - `device-f103/App/register_map`
  - `device-f103/App/modbus_slave`
  - `device-f103/App/fault_injection`
- 源文件：
  - `shared/registers/regs_gen.c`
  - `third_party/nanomodbus/nanomodbus.c`
  - `device-f103/App/**/*.c`（`rs485_port.c` 由你补）
- `board_config.h` 未填写前会 `#error`，这是故意的。

## 接到 CubeMX 生成的 main.c

```c
/* USER CODE BEGIN 2 */
app_init(HAL_GetTick());
/* USER CODE END 2 */

while (1) {
    app_poll(HAL_GetTick());
}
```

在 `stm32f1xx_it.c` 对应的 `USARTx_IRQHandler` 里调用：

```c
rs485_port_irq_handler();
```

注意：`app_init()` 必须在 GPIO/DMA/USART 初始化之后调用。

## 验证命令

```bash
python tools/register_map/gen_registers.py
python tests/host/test_crc16.py
python tools/modbus_master_tester/master_tester.py --tcp 127.0.0.1:15020 --slave 1 --function 3 --address 0 --count 4 --iterations 5
```

## 说明

`mb_regs.c` / `mb_slave.c` 是按 C99 写的，但本机没有 `gcc`/ARM GCC，尚未编译。导入 Keil/CubeIDE 后先编译，再按错误逐条修；不要直接上硬件。
