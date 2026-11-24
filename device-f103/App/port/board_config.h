#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * 板级配置：以下为 W1 的占位方案，接线前必须核对实际硬件。
 *
 * 假设：
 * - F103C8T6 最小系统板（64KB Flash / 20KB RAM）；
 * - MAX3485/SP3485 3.3V 模块，USART2：TX=PA2，RX=PA3；
 * - DE/RE 由 PA8 控制，高电平发送；
 * - 调试串口 USART1：PA9/PA10，115200。
 *
 * 如果实际接线不同，只改本文件，不要改 rs485_port.c 的时序。
 * 注意：正点原子探索者 V3 是 F407 板，不含 F103；F103 从站需单独一块
 * C8T6 最小系统板，或用 USB-RS485 + PC 模拟从站先开发主站。
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
