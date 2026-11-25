# 准备期清单（11/25–12/13）与 W1 移交

> 更新：2026-11-25。代码已在 `main`，无未推送分支。
> 准备期目标：12/13 前完成 **3 项硬件确认 + 寄存器表评审**，12/14 按 `w1-execution-plan.md` 进 W1。
> CET-4 优先：12/12 前每天 ≤1h，只做确认类工作，不写代码。

---

## 0. 准备期状态

| 项 | 负责 | 状态 |
|---|---|---|
| 寄存器表 18 点 + 生成器 | AI | ✅ 已评审通过（11/25 两项修订，见 §2） |
| CRC16 测试向量 | AI | ✅ 14 项全过（标准向量 0x4B37） |
| 主站测试工具（TCP 回环） | AI | ✅ 读 5/5、写回显 0x04D2 正确 |
| Python 依赖锁定 | AI | ✅ 修正为 `pymodbus>=3.9,<4`（3.11.4 实测可用） |
| 文档一致性扫描 | AI | ✅ 无 9600 / 3 从站 / LAN8720 类矛盾 |
| 硬件 3 问 | 你 | 🔶 1/3（F407 已定，F103 侧待填） |
| W1 任务卡确认 | 你 | ⬜ 待确认 |

---

## 1. 硬件 3 问

| # | 问题 | 答案 |
|---|---|---|
| 1 | RS485 模块是手动 DE/RE，还是自动收发？（F103 侧） | ⬜ 待填 |
| 2 | 若手动：DE 高电平还是低电平发送？（F103 侧） | ⬜ 待填 |
| 3 | F407 板载 RS485 的 DE 引脚？ | ✅ **PG8**，DE 与 /RE 短接，**高 = 发送、低 = 接收**；USART2 PA2/PA3；P4 跳线拨 RS485 侧（依据 `hardware-f407-v3.md`） |

> 已定默认（可在 `board_config.h` 覆盖）：F103 用 USART2（PA2 TX→DI、PA3 RX←RO），DE/RE→PA1；调试口 USART1（PA9/PA10）。模块必须 3.3V。F103 侧模块型号与 DE 极性仍待确认。
> F407 端已板载 120Ω 终端，F103 端再加一个 120Ω 即两端齐；短距离桌面可先只保留板载端。

---

## 2. 准备期两项修订（11/25 已定）

| 点位 | 原值 | 新值 | 原因 |
|---|---|---|---|
| Slave 2 `0x0010` 清零命令 | W | **RW** | 读恒为 0、写 1 清零；避免主站批量读参数被 0x02 异常打断 |
| Slave 4 运行计数镜像 | stats `0x03C0` | **measurement `0x01C0`** | 测量类点位统一进测量分区，统计区留给通信统计 |

已重跑生成器：`shared/registers/regs_gen.h/.c`、`tools/modbus_master_tester/regs_gen.py`、`docs/register-map.generated.md` 全部同步，18 点校验通过。

---

## 3. AI 侧已交付（复核结果 2026-11-25）

| 项 | 文件 | 复核结果 |
|---|---|---|
| 协议栈引入 | `third_party/nanomodbus/`（MIT） | 源码与 LICENSE 齐全 |
| 寄存器表单一来源 | `docs/register-map.csv`（18 点） | 生成器校验通过 |
| 生成器 | `tools/register_map/gen_registers.py` | 生成 C/Python/Markdown |
| C 寄存器表 | `shared/registers/regs_gen.h/.c` | 地址/镜像区无重叠 |
| Python 寄存器表 | `tools/modbus_master_tester/regs_gen.py` | 可导入、lookup 正常 |
| CRC16 测试 | `tests/host/test_crc16.py` | 14 项全过 |
| 主站测试工具 | `tools/modbus_master_tester/master_tester.py` | TCP 回环读 5/5、写回显正确 |
| 寄存器模型 | `device-f103/App/register_map/mb_regs.h/.c` | 未编译（本机无 ARM 工具链） |
| 从站适配 | `device-f103/App/modbus_slave/mb_slave.h/.c` | 未编译；含异常响应与故障注入 |
| 4 从站路由 | `device-f103/App/modbus_slave/mb_router.h/.c` | 未编译；单播 + 广播写 + 轮询 |
| 故障注入 | `device-f103/App/fault_injection/fault_injection.h/.c` | 未编译；6 种模式，NO_RESPONSE 30s 自恢复 |
| 应用层 | `device-f103/App/app_f103.h/.c` | 未编译；模拟传感器 + 主循环 |
| RS485 接口 | `device-f103/App/port/rs485_port.h` | 仅接口 |
| RS485 实现规格 | `docs/rs485-port-spec.md` | 时序、错误处理、验收 |
| 板级配置模板 | `device-f103/App/port/board_config.h` | 未填写，故意 `#error` |

---

## 4. 留给你的部分（W1 红区）

1. **填写 `board_config.h`**：F103 型号、USART 实例、TX/RX 引脚、DE/RE 引脚、是否自动收发。
2. **实现 `rs485_port.c`**：DMA + IDLE 判帧、DE/RE 等 UART TC、错误计数。红区，必须你写。
3. **把 `app_init()` / `app_poll()` 接到 CubeMX 生成的 `main.c`**：`now_ms` 用 `HAL_GetTick()`。
4. **硬件联调**：先 USB-RS485 + PC 主站脚本，再接 F407。
5. **审查 `mb_regs.c` / `fault_injection.c`**：确认权限、清零命令、故障模式符合预期。

---

## 5. 编译配置（Keil/CubeIDE）

Include 路径：

- `third_party/nanomodbus`
- `shared/registers`
- `device-f103/App`
- `device-f103/App/port`
- `device-f103/App/register_map`
- `device-f103/App/modbus_slave`
- `device-f103/App/fault_injection`

源文件：

- `shared/registers/regs_gen.c`
- `third_party/nanomodbus/nanomodbus.c`
- `device-f103/App/**/*.c`（`rs485_port.c` 由你补）

`board_config.h` 未填写前会 `#error`，这是故意的。

---

## 6. 接到 CubeMX 生成的 main.c

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

---

## 7. 验证命令

```bash
python -m pip install -r tools/modbus_master_tester/requirements.txt
python tools/register_map/gen_registers.py
python tests/host/test_crc16.py
python tests/host/tcp_slave_stub.py --port 15020      # 另开一个终端
python tools/modbus_master_tester/master_tester.py --tcp 127.0.0.1:15020 --slave 1 --function 4 --address 0 --count 4 --iterations 5
```

---

## 8. 说明

`mb_regs.c` / `mb_slave.c` 按 C99 写，但本机没有 ARM GCC，尚未编译。导入 Keil/CubeIDE 后先编译，再按错误逐条修；不要直接上硬件。
