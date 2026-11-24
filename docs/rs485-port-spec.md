# RS485 传输层实现规格（你来写代码）

> 目标：实现 `device-f103/App/port/rs485_port.c`，满足 `rs485_port.h` 的接口。
> 这是红区：面试会追问 DE/RE 时序和判帧，必须你自己写、自己调。

## 1. 硬件假设

- USART 由 CubeMX 初始化，`huart1/2/3` 已存在。
- RX 使用 DMA 通道，TX 使用 DMA 通道（或阻塞发送，W1 可先阻塞）。
- DE/RE 由 GPIO 控制；若模块自动收发，`BOARD_RS485_AUTO_DIRECTION=1` 时跳过 GPIO。
- 调试串口与 RS485 串口分开。

## 2. RX：DMA + IDLE 判帧

1. `rs485_port_init()`：
   - 根据 `BOARD_RS485_UART_INDEX` 选择 `&huart1/2/3`；
   - 配置 DE 引脚为输出，默认接收；
   - `HAL_UART_Receive_DMA(uart, s_rx_buf, sizeof(s_rx_buf))`；
   - `__HAL_UART_ENABLE_IT(uart, UART_IT_IDLE)`。
2. `rs485_port_irq_handler()`（在 `USARTx_IRQHandler` 里调用）：
   - 判断 `__HAL_UART_GET_FLAG(uart, UART_FLAG_IDLE)`；
   - `__HAL_UART_CLEAR_IDLEFLAG(uart)`；
   - 停止 RX DMA，读 `__HAL_DMA_GET_COUNTER(uart->hdmarx)`；
   - `len = sizeof(s_rx_buf) - remaining`；
   - `len >= 4` 才入队，否则丢弃并计数；
   - 重新 `HAL_UART_Receive_DMA`。
3. 队列深度 4；满时丢最旧帧，`rx_dropped++`。

## 3. TX：DE 时序

1. 检查上一帧是否发完；忙则返回 `-1`（上层会重试/计数）。
2. 拉高 DE（若 `DE_ACTIVE_HIGH=1`）。
3. 启动发送：
   - W1 可先用 `HAL_UART_Transmit(uart, buf, count, timeout)`；
   - 或 DMA 发送 + 轮询 `UART_FLAG_TC`。
4. **必须等到 TC**（最后一个停止位发完），再拉低 DE。
5. `tx_frames++`。

常见错误：DMA 完成中断就拉低 DE，最后一字节被截断。TC 才是发送完成的标志。

## 4. 错误处理

- ORE/FE/NE：清 `SR/DR`，`uart_errors++`，重启 RX DMA。
- IDLE 中断里不要做 CRC/协议解析，只入队。
- 队列满、长度异常、CRC 错误分别计数。

## 5. 主循环

```c
void app_poll(uint32_t now_ms) {
    rs485_port_tick();      /* TX 完成、DE 释放、超时 */
    mb_router_poll(now_ms); /* 取帧 -> 分发 -> 4 个 server */
    fault_tick(now_ms);
}
```

`now_ms` 用 `HAL_GetTick()`。

## 6. 验收

- 逻辑分析仪：一帧结束后 DE 在 TC 之后才拉低。
- 115200 下连续 1000 次 0x03/0x04 读无错。
- 人为缩短 DE 释放时间，能复现最后一字节错误（说明 TC 判断有效）。
- 队列满、CRC 错、UART 错误计数能在调试串口打印。

## 7. 参考代码结构（不要照抄，按你的 HAL 版本调整）

```c
static UART_HandleTypeDef *s_uart;
static uint8_t s_rx_buf[RS485_FRAME_MAX];
static Rs485Frame s_queue[4];
static volatile uint8_t s_head, s_tail;

void rs485_port_init(void) {
    s_uart = &huart1; /* 按 board_config 选择 */
    de_set(0);
    HAL_UART_Receive_DMA(s_uart, s_rx_buf, sizeof(s_rx_buf));
    __HAL_UART_ENABLE_IT(s_uart, UART_IT_IDLE);
}

void rs485_port_irq_handler(void) {
    if (__HAL_UART_GET_FLAG(s_uart, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(s_uart);
        HAL_UART_DMAStop(s_uart);
        uint16_t remaining = __HAL_DMA_GET_COUNTER(s_uart->hdmarx);
        uint16_t len = sizeof(s_rx_buf) - remaining;
        push_frame(s_rx_buf, len);
        HAL_UART_Receive_DMA(s_uart, s_rx_buf, sizeof(s_rx_buf));
    }
}
```

## 8. 需要你确认的板级参数

- F103 型号与 Flash/RAM；
- USART 实例与 TX/RX 引脚；
- DE/RE 引脚与有效电平；
- 模块是否自动收发；
- 调试串口。
