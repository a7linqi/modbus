#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * 板级配置（2026-11-25 定稿）：接线以 docs/w1-status.md §1 采购决定为准。
 *
 * 定稿依据：
 * - F103C8T6 最小系统板（64KB Flash / 20KB RAM）；
 * - MAX3485 3.3V 模块（手动 DE/RE 款），USART2：TX=PA2，RX=PA3；
 * - DE 与 RE 短接后由 PA8 控制，高电平发送（MAX3485 标准行为）；
 * - 调试串口 USART1：PA9/PA10，115200。
 *
 * 如果实际接线不同，只改本文件，不要改 rs485_port.c 的时序。
 * 模块到货后核对：丝印 MAX3485/SP3485，引脚含 DE/RE，VCC 接 3.3V。
 */

#define BOARD_RS485_UART_INDEX 2 /* 1=USART1, 2=USART2, 3=USART3 */
#define BOARD_RS485_TX_PORT 'A'  /* PA2 */
#define BOARD_RS485_TX_PIN 2
#define BOARD_RS485_RX_PORT 'A'  /* PA3 */
#define BOARD_RS485_RX_PIN 3

#define BOARD_RS485_DE_PORT 'A' /* PA8 */
#define BOARD_RS485_DE_PIN 8
#define BOARD_RS485_DE_ACTIVE_HIGH 1 /* DE 高电平发送=1；反相=0 */
#define BOARD_RS485_AUTO_DIRECTION 0 /* 模块自动收发=1，需要 GPIO 控制=0 */

#define BOARD_DEBUG_UART_INDEX 1 /* USART1：PA9/PA10 */
#define BOARD_DEBUG_BAUD 115200U

#define BOARD_MCU_FLASH_KB 64
#define BOARD_MCU_RAM_KB 20

#if BOARD_RS485_UART_INDEX == 0
#error "请先在 board_config.h 填写 BOARD_RS485_UART_INDEX 与 TX/RX/DE 引脚"
#endif

#if BOARD_RS485_TX_PIN == 0xFF || BOARD_RS485_RX_PIN == 0xFF
#error "请填写 BOARD_RS485_TX_PIN / BOARD_RS485_RX_PIN"
#endif

#if BOARD_RS485_AUTO_DIRECTION == 0 && BOARD_RS485_DE_PIN == 0xFF
#error "请填写 BOARD_RS485_DE_PIN，或把 BOARD_RS485_AUTO_DIRECTION 设为 1"
#endif

#endif /* BOARD_CONFIG_H */
