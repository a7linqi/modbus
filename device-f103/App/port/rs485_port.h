#ifndef RS485_PORT_H
#define RS485_PORT_H

#include <stdbool.h>
#include <stdint.h>

#define RS485_FRAME_MAX 256U

typedef struct {
    uint8_t data[RS485_FRAME_MAX];
    uint16_t len;
} Rs485Frame;

/*
 * RS485 传输层：整帧收发，不暴露 HAL。
 *
 * 实现要求：
 * - RX 用 DMA + UART IDLE，整帧收完再入队；
 * - TX 先拉 DE，DMA 发完后继续等 UART TC，再拉低 DE；
 * - 中断里只做收帧/入队，不做协议解析；
 * - 队列满时丢最旧帧并计数，不能阻塞主循环。
 */

void rs485_port_init(void);

/* 取一帧；timeout_ms=0 表示非阻塞。返回 true 表示取到。 */
bool rs485_port_read_frame(Rs485Frame *frame, uint32_t timeout_ms);

/* 发送一帧；内部负责 DE/RE 与 TC 等待。返回实际字节数，<0 错误。 */
int32_t rs485_port_write(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg);

/* 主循环调用：处理 TX 完成、DE 释放、超时与错误统计。 */
void rs485_port_tick(void);

/* 由 USARTx_IRQHandler 调用：检测 IDLE，收帧入队。 */
void rs485_port_irq_handler(void);

typedef struct {
    uint32_t rx_frames;
    uint32_t rx_dropped;
    uint32_t tx_frames;
    uint32_t crc_errors;
    uint32_t uart_errors;
} Rs485PortStats;

const Rs485PortStats *rs485_port_stats(void);

#endif /* RS485_PORT_H */
