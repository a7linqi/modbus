#ifndef MB_REGS_H
#define MB_REGS_H

#include <stdbool.h>
#include <stdint.h>

#include "regs_gen.h"

#define MB_SLAVE_COUNT 4U
#define MB_REG_SPACE_SIZE 256U

typedef enum {
    MB_REGS_OK = 0,
    MB_REGS_ERR_ADDR = 1,   /* 未定义地址或越界，映射异常码 0x02 */
    MB_REGS_ERR_ACCESS = 2, /* 只读/只写冲突，映射异常码 0x02 */
    MB_REGS_ERR_VALUE = 3   /* 数量或数值非法，映射异常码 0x03 */
} MbRegsStatus;

/* 上电初始化：清零并按 register-map.csv 的默认值填充保持寄存器。 */
void mb_regs_init(void);

/* Modbus 读写入口；address 为 0-based 协议地址。 */
MbRegsStatus mb_regs_read(uint8_t slave_id, MbRegSpace space, uint16_t address,
                          uint16_t quantity, uint16_t *out);
MbRegsStatus mb_regs_write(uint8_t slave_id, uint16_t address,
                           const uint16_t *values, uint16_t quantity);

/* 内部更新输入寄存器（传感器模拟、运行计数、电量累计）。不检查权限。 */
bool mb_regs_set_input(uint8_t slave_id, uint16_t address, uint16_t value);
bool mb_regs_set_input_u32(uint8_t slave_id, uint16_t address, uint32_t value);

/* 供故障注入/调试读取当前保持寄存器值。 */
bool mb_regs_get_holding(uint8_t slave_id, uint16_t address, uint16_t *out);

#endif /* MB_REGS_H */
