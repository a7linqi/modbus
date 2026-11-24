#ifndef APP_F103_H
#define APP_F103_H

#include <stdint.h>

/* 上电初始化：寄存器、故障注入、RS485、4 个逻辑从站。 */
void app_init(uint32_t now_ms);

/* 主循环调用：RS485 tick、路由轮询、模拟传感器更新。 */
void app_poll(uint32_t now_ms);

#endif /* APP_F103_H */
