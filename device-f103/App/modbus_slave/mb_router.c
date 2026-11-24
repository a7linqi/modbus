#include "mb_router.h"

#include "mb_regs.h"
#include "rs485_port.h"

static MbSlaveCtx s_ctx[MB_SLAVE_COUNT];

static bool frame_crc_ok(const Rs485Frame *frame) {
    if (frame->len < 4U) {
        return false;
    }
    uint16_t crc = nmbs_crc_calc(frame->data, (uint32_t)(frame->len - 2U), 0);
    return frame->data[frame->len - 2U] == (uint8_t)(crc >> 8) &&
           frame->data[frame->len - 1U] == (uint8_t)(crc & 0xFFU);
}

/* 广播只执行写，不响应；地址 0 不进入 nanoMODBUS server。 */
static void handle_broadcast(const Rs485Frame *frame) {
    if (!frame_crc_ok(frame)) {
        return;
    }

    uint8_t function = frame->data[1];
    uint16_t address = (uint16_t)(((uint16_t)frame->data[2] << 8) | frame->data[3]);

    if (function == 0x06U && frame->len == 8U) {
        uint16_t value = (uint16_t)(((uint16_t)frame->data[4] << 8) | frame->data[5]);
        for (uint8_t slave = 1U; slave <= MB_SLAVE_COUNT; ++slave) {
            (void)mb_regs_write(slave, address, &value, 1U);
        }
        return;
    }

    if (function == 0x10U && frame->len >= 11U) {
        uint16_t quantity = (uint16_t)(((uint16_t)frame->data[4] << 8) | frame->data[5]);
        uint8_t byte_count = frame->data[6];
        if (quantity == 0U || quantity > 16U || byte_count != (uint8_t)(quantity * 2U) ||
            frame->len != (uint16_t)(9U + quantity * 2U)) {
            return;
        }

        uint16_t values[16];
        for (uint16_t i = 0; i < quantity; ++i) {
            values[i] = (uint16_t)(((uint16_t)frame->data[7U + 2U * i] << 8) |
                                   frame->data[8U + 2U * i]);
        }
        for (uint8_t slave = 1U; slave <= MB_SLAVE_COUNT; ++slave) {
            (void)mb_regs_write(slave, address, values, quantity);
        }
    }
}

void mb_router_init(void) {
    for (uint8_t i = 0; i < MB_SLAVE_COUNT; ++i) {
        (void)mb_slave_init(&s_ctx[i], (uint8_t)(i + 1U));
    }
}

void mb_router_poll(uint32_t now_ms) {
    Rs485Frame frame;
    while (rs485_port_read_frame(&frame, 0U)) {
        if (frame.len < 2U) {
            continue;
        }
        uint8_t slave_id = frame.data[0];
        if (slave_id == 0U) {
            handle_broadcast(&frame);
        } else if (slave_id >= 1U && slave_id <= MB_SLAVE_COUNT) {
            mb_slave_on_frame(&s_ctx[slave_id - 1U], frame.data, frame.len);
        }
    }

    for (uint8_t i = 0; i < MB_SLAVE_COUNT; ++i) {
        mb_slave_poll(&s_ctx[i], now_ms);
    }
}

MbSlaveCtx *mb_router_ctx(uint8_t slave_id) {
    if (slave_id < 1U || slave_id > MB_SLAVE_COUNT) {
        return 0;
    }
    return &s_ctx[slave_id - 1U];
}
