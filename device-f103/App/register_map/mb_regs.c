#include "mb_regs.h"

#include <string.h>

static uint16_t s_input[MB_SLAVE_COUNT][MB_REG_SPACE_SIZE];
static uint16_t s_holding[MB_SLAVE_COUNT][MB_REG_SPACE_SIZE];

static bool slave_valid(uint8_t slave_id) {
    return slave_id >= 1U && slave_id <= MB_SLAVE_COUNT;
}

static const MbRegPoint *find_point(uint8_t slave_id, MbRegSpace space, uint16_t address) {
    for (uint16_t i = 0; i < MB_REG_POINT_COUNT; ++i) {
        const MbRegPoint *p = &g_mb_reg_points[i];
        if (p->slave_id != slave_id || p->space != (uint8_t)space) {
            continue;
        }
        if (address >= p->address && address < (uint16_t)(p->address + p->reg_count)) {
            return p;
        }
    }
    return NULL;
}

void mb_regs_init(void) {
    memset(s_input, 0, sizeof(s_input));
    memset(s_holding, 0, sizeof(s_holding));

    for (uint16_t i = 0; i < MB_REG_POINT_COUNT; ++i) {
        const MbRegPoint *p = &g_mb_reg_points[i];
        if (p->space == (uint8_t)MB_REG_SPACE_HOLDING && p->slave_id <= MB_SLAVE_COUNT) {
            s_holding[p->slave_id - 1U][p->address] = p->default_value;
        }
    }
}

MbRegsStatus mb_regs_read(uint8_t slave_id, MbRegSpace space, uint16_t address,
                          uint16_t quantity, uint16_t *out) {
    if (!slave_valid(slave_id) || out == NULL || quantity == 0U || quantity > 125U) {
        return MB_REGS_ERR_VALUE;
    }
    if ((uint32_t)address + quantity > MB_REG_SPACE_SIZE) {
        return MB_REGS_ERR_ADDR;
    }

    for (uint16_t offset = 0; offset < quantity; ++offset) {
        if (find_point(slave_id, space, (uint16_t)(address + offset)) == NULL) {
            return MB_REGS_ERR_ADDR;
        }
    }

    const uint16_t *src = (space == MB_REG_SPACE_INPUT)
                              ? &s_input[slave_id - 1U][address]
                              : &s_holding[slave_id - 1U][address];
    memcpy(out, src, (size_t)quantity * sizeof(uint16_t));
    return MB_REGS_OK;
}

MbRegsStatus mb_regs_write(uint8_t slave_id, uint16_t address,
                           const uint16_t *values, uint16_t quantity) {
    if (!slave_valid(slave_id) || values == NULL || quantity == 0U || quantity > 123U) {
        return MB_REGS_ERR_VALUE;
    }
    if ((uint32_t)address + quantity > MB_REG_SPACE_SIZE) {
        return MB_REGS_ERR_ADDR;
    }

    for (uint16_t offset = 0; offset < quantity; ++offset) {
        const MbRegPoint *p = find_point(slave_id, MB_REG_SPACE_HOLDING, (uint16_t)(address + offset));
        if (p == NULL) {
            return MB_REGS_ERR_ADDR;
        }
        if (p->access == (uint8_t)MB_REG_ACCESS_R) {
            return MB_REGS_ERR_ACCESS;
        }
    }

    for (uint16_t offset = 0; offset < quantity; ++offset) {
        s_holding[slave_id - 1U][address + offset] = values[offset];
    }

    /* Slave2 0x0010：写 1 清零累计电量；无论写什么，读回都是 0。 */
    if (slave_id == 2U && address == MB_REG_S2_HOLDING_CLEAR_COMMAND_ADDR) {
        if (values[0] == 1U) {
            s_input[1][MB_REG_S2_INPUT_ENERGY_ADDR] = 0U;
            s_input[1][MB_REG_S2_INPUT_ENERGY_ADDR + 1U] = 0U;
        }
        s_holding[1][MB_REG_S2_HOLDING_CLEAR_COMMAND_ADDR] = 0U;
    }

    return MB_REGS_OK;
}

bool mb_regs_set_input(uint8_t slave_id, uint16_t address, uint16_t value) {
    if (!slave_valid(slave_id) || address >= MB_REG_SPACE_SIZE) {
        return false;
    }
    s_input[slave_id - 1U][address] = value;
    return true;
}

bool mb_regs_set_input_u32(uint8_t slave_id, uint16_t address, uint32_t value) {
    if (!slave_valid(slave_id) || (uint32_t)address + 2U > MB_REG_SPACE_SIZE || (address & 1U) != 0U) {
        return false;
    }
    /* ABCD：高字在前。 */
    s_input[slave_id - 1U][address] = (uint16_t)(value >> 16);
    s_input[slave_id - 1U][address + 1U] = (uint16_t)(value & 0xFFFFU);
    return true;
}

bool mb_regs_get_holding(uint8_t slave_id, uint16_t address, uint16_t *out) {
    if (!slave_valid(slave_id) || out == NULL || address >= MB_REG_SPACE_SIZE) {
        return false;
    }
    if (find_point(slave_id, MB_REG_SPACE_HOLDING, address) == NULL) {
        return false;
    }
    *out = s_holding[slave_id - 1U][address];
    return true;
}
