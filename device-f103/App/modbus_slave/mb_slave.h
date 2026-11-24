#ifndef MB_SLAVE_H
#define MB_SLAVE_H

#include <stdbool.h>
#include <stdint.h>

#include "nanomodbus.h"

#define MB_FRAME_MAX 256U

/*
 * 一个逻辑从站对应一个 nanoMODBUS server 实例。
 * 4 个逻辑从站共享同一路 RS485：rs485_port 收到整帧后，按帧首地址
 * 分发给对应 ctx；每个 ctx 的 platform->read 只从自己的 rx_frame 取字节。
 *
 * 这样既保留库的协议处理，又能让 4 个 Slave ID 在同一块 F103 上工作。
 */
typedef struct {
    uint8_t slave_id;
    nmbs_t server;
    uint8_t rx_frame[MB_FRAME_MAX];
    uint16_t rx_len;
    uint16_t rx_pos;
    volatile bool rx_ready;
    bool delay_started;
    uint32_t delay_until_ms;
    uint32_t request_count;
    uint32_t error_count;
} MbSlaveCtx;

bool mb_slave_init(MbSlaveCtx *ctx, uint8_t slave_id);

/* 由 rs485 分发器调用：复制一整帧，置 rx_ready。 */
void mb_slave_on_frame(MbSlaveCtx *ctx, const uint8_t *frame, uint16_t len);

/* 主循环调用：内部调用 nmbs_server_poll，并应用故障注入。 */
void mb_slave_poll(MbSlaveCtx *ctx, uint32_t now_ms);

#endif /* MB_SLAVE_H */
