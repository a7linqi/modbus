# 共享寄存器表（生成物）

- 单一事实源：`docs/register-map.csv`
- 生成器：`tools/register_map/gen_registers.py`
- 生成物：
  - `regs_gen.h`：点位宏与 `MbRegPoint` 声明
  - `regs_gen.c`：`g_mb_reg_points[]` 定义
  - `docs/register-map.generated.md`：可读表
  - `tools/modbus_master_tester/regs_gen.py`：Python 常量

## 使用

F103 与 F407 工程都把 `shared/registers/regs_gen.c` 加入编译，并包含 `regs_gen.h`。

```c
#include "regs_gen.h"

uint16_t addr = MB_REG_S1_INPUT_TEMPERATURE_ADDR;
uint16_t mirror = MB_REG_S1_INPUT_TEMPERATURE_MIRROR;
```

## 修改规则

1. 只改 `docs/register-map.csv`；
2. 运行 `python tools/register_map/gen_registers.py`；
3. 不要手改 `regs_gen.h/.c` 或 `regs_gen.py`；
4. 生成器会校验地址重叠、u32 对齐、镜像区重叠、类型/权限合法性。
