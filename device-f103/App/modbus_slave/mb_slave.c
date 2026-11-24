#include "mb_slave.h"

#include <string.h>

#include "fault_injection.h"
#include "mb_regs.h"
#include "rs485_port.h"

static int32_t platform_read(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg) {
    (void)byte_timeout_ms;
    MbSlaveCtx *ctx = (MbSlaveCtx *)arg;
    if (ctx == 0 || !ctx->rx_ready || ctx->rx_pos >= ctx->rx_len) {
        return 0;
    }

    uint16_t remain = (uint16_t)(ctx->rx_len - ctx->rx_pos);
    uint16_t take = (count < remain) ? count : remain;
    memcpy(buf, &ctx->rx_frame[ctx->rx_pos], take);
    ctx->rx_pos = (uint16_t)(ctx->rx_pos + take);
    return (int32_t)take;
}

static int32_t platform_write(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg) {
    MbSlaveCtx *ctx = (MbSlaveCtx *)arg;
    if (ctx == 0 || count > RS485_FRAME_MAX) {
        return -1;
    }

    if (fault_bad_crc(ctx->slave_id) || fault_truncate(ctx->slave_id)) {
        uint8_t tmp[RS485_FRAME_MAX];
        uint16_t len = count;
        memcpy(tmp, buf, count);
        if (fault_truncate(ctx->slave_id) && len > 0U) {
            len--;
        }
        if (fault_bad_crc(ctx->slave_id) && len >= 2U) {
            tmp[len - 1U] ^= 0xFFU;
        }
        return rs485_port_write(tmp, len, byte_timeout_ms, 0);
    }

    return rs485_port_write(buf, count, byte_timeout_ms, 0);
}

static nmbs_error map_regs_status(MbRegsStatus status) {
    switch (status) {
        case MB_REGS_OK:
            return NMBS_ERROR_NONE;
        case MB_REGS_ERR_VALUE:
            return NMBS_EXCEPTION_ILLEGAL_DATA_VALUE;
        case MB_REGS_ERR_ADDR:
        case MB_REGS_ERR_ACCESS:
        default:
            return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }
}

static nmbs_error cb_read_holding(uint16_t address, uint16_t quantity, uint16_t *registers_out,
                                  uint8_t unit_id, void *arg) {
    (void)unit_id;
    MbSlaveCtx *ctx = (MbSlaveCtx *)arg;
    return map_regs_status(mb_regs_read(ctx->slave_id, MB_REG_SPACE_HOLDING, address, quantity, registers_out));
}

static nmbs_error cb_read_input(uint16_t address, uint16_t quantity, uint16_t *registers_out,
                                uint8_t unit_id, void *arg) {
    (void)unit_id;
    MbSlaveCtx *ctx = (MbSlaveCtx *)arg;
    return map_regs_status(mb_regs_read(ctx->slave_id, MB_REG_SPACE_INPUT, address, quantity, registers_out));
}

static nmbs_error cb_write_single(uint16_t address, uint16_t value, uint8_t unit_id, void *arg) {
    (void)unit_id;
    MbSlaveCtx *ctx = (MbSlaveCtx *)arg;
    return map_regs_status(mb_regs_write(ctx->slave_id, address, &value, 1U));
}

static nmbs_error cb_write_multiple(uint16_t address, uint16_t quantity, const uint16_t *registers,
                                    uint8_t unit_id, void *arg) {
    (void)unit_id;
    MbSlaveCtx *ctx = (MbSlaveCtx *)arg;
    return map_regs_status(mb_regs_write(ctx->slave_id, address, registers, quantity));
}

bool mb_slave_init(MbSlaveCtx *ctx, uint8_t slave_id) {
    if (ctx == 0 || slave_id < 1U || slave_id > MB_SLAVE_COUNT) {
        return false;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->slave_id = slave_id;

    nmbs_platform_conf platform;
    nmbs_platform_conf_create(&platform);
    platform.transport = NMBS_TRANSPORT_RTU;
    platform.read = platform_read;
    platform.write = platform_write;
    platform.arg = ctx;

    nmbs_callbacks callbacks;
    nmbs_callbacks_create(&callbacks);
    callbacks.read_holding_registers = cb_read_holding;
    callbacks.read_input_registers = cb_read_input;
    callbacks.write_single_register = cb_write_single;
    callbacks.write_multiple_registers = cb_write_multiple;

    nmbs_error err = nmbs_server_create(&ctx->server, slave_id, &platform, &callbacks);
    if (err != NMBS_ERROR_NONE) {
        ctx->error_count++;
        return false;
    }
    return true;
}

static void drop_frame(MbSlaveCtx *ctx) {
    ctx->rx_ready = false;
    ctx->rx_len = 0;
    ctx->rx_pos = 0;
    ctx->delay_started = false;
}

static void send_forced_exception(MbSlaveCtx *ctx) {
    uint8_t resp[5];
    resp[0] = ctx->slave_id;
    resp[1] = (uint8_t)(ctx->rx_frame[1] | 0x80U);
    resp[2] = 0x04U; /* 从站设备故障 */
    uint16_t crc = nmbs_crc_calc(resp, 3U, 0);
    resp[3] = (uint8_t)(crc >> 8);
    resp[4] = (uint8_t)(crc & 0xFFU);
    (void)rs485_port_write(resp, sizeof(resp), 50, 0);
}

void mb_slave_on_frame(MbSlaveCtx *ctx, const uint8_t *frame, uint16_t len) {
    if (ctx == 0 || frame == 0 || len == 0U || len > MB_FRAME_MAX) {
        return;
    }
    if (ctx->rx_ready) {
        ctx->error_count++; /* 上一帧尚未处理，记为丢弃 */
    }
    memcpy(ctx->rx_frame, frame, len);
    ctx->rx_len = len;
    ctx->rx_pos = 0;
    ctx->delay_started = false;
    ctx->rx_ready = true;
}

void mb_slave_poll(MbSlaveCtx *ctx, uint32_t now_ms) {
    if (ctx == 0 || !ctx->rx_ready) {
        return;
    }

    if (ctx->rx_len < 2U) {
        drop_frame(ctx);
        return;
    }

    ctx->request_count++;

    if (fault_drop_response(ctx->slave_id)) {
        drop_frame(ctx);
        return;
    }

    uint32_t delay = fault_delay_ms(ctx->slave_id);
    if (delay > 0U) {
        if (!ctx->delay_started) {
            ctx->delay_started = true;
            ctx->delay_until_ms = now_ms + delay;
            return;
        }
        if ((int32_t)(now_ms - ctx->delay_until_ms) < 0) {
            return;
        }
        ctx->delay_started = false;
    }

    if (fault_force_exception(ctx->slave_id)) {
        send_forced_exception(ctx);
        drop_frame(ctx);
        return;
    }

    nmbs_error err = nmbs_server_poll(&ctx->server);
    if (err != NMBS_ERROR_NONE && err != NMBS_ERROR_TIMEOUT) {
        ctx->error_count++;
    }

    /* 一帧只处理一次；未消费完的残帧直接丢弃，等待下一帧。 */
    drop_frame(ctx);
}
