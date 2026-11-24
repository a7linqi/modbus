#ifndef MB_ROUTER_H
#define MB_ROUTER_H

#include <stdint.h>

#include "mb_slave.h"

/* 初始化 4 个逻辑从站；mb_regs_init() 由 app_init 先调用。 */
void mb_router_init(void);

/* 主循环：取整帧 -> 按首字节分发 -> 轮询 4 个 server。 */
void mb_router_poll(uint32_t now_ms);

MbSlaveCtx *mb_router_ctx(uint8_t slave_id);

#endif /* MB_ROUTER_H */
