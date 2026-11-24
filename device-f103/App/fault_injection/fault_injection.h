#ifndef FAULT_INJECTION_H
#define FAULT_INJECTION_H

#include <stdbool.h>
#include <stdint.h>

#define FAULT_SLAVE_MAX 4U
#define FAULT_NO_RESPONSE_TIMEOUT_MS 30000U
#define FAULT_DELAY_DEFAULT_MS 500U

typedef enum {
    FAULT_NONE = 0,
    FAULT_NO_RESPONSE,
    FAULT_BAD_CRC,
    FAULT_DELAY_RESPONSE,
    FAULT_EXCEPTION_CODE,
    FAULT_TRUNCATED_FRAME,
    FAULT_MODE_COUNT
} FaultMode;

void fault_init(uint32_t now_ms);
void fault_set(uint8_t slave_id, FaultMode mode, uint32_t now_ms);
FaultMode fault_get(uint8_t slave_id);

/* 30s 自动清除 NO_RESPONSE；其余模式保持到显式改写。 */
void fault_tick(uint32_t now_ms);

bool fault_drop_response(uint8_t slave_id);
bool fault_bad_crc(uint8_t slave_id);
bool fault_truncate(uint8_t slave_id);
bool fault_force_exception(uint8_t slave_id);
uint32_t fault_delay_ms(uint8_t slave_id);

#endif /* FAULT_INJECTION_H */
