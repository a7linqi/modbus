#include "fault_injection.h"

static FaultMode s_mode[FAULT_SLAVE_MAX];
static uint32_t s_set_ms[FAULT_SLAVE_MAX];

static bool slave_valid(uint8_t slave_id) {
    return slave_id >= 1U && slave_id <= FAULT_SLAVE_MAX;
}

void fault_init(uint32_t now_ms) {
    (void)now_ms;
    for (uint8_t i = 0; i < FAULT_SLAVE_MAX; ++i) {
        s_mode[i] = FAULT_NONE;
        s_set_ms[i] = 0U;
    }
}

void fault_set(uint8_t slave_id, FaultMode mode, uint32_t now_ms) {
    if (!slave_valid(slave_id) || mode >= FAULT_MODE_COUNT) {
        return;
    }
    s_mode[slave_id - 1U] = mode;
    s_set_ms[slave_id - 1U] = now_ms;
}

FaultMode fault_get(uint8_t slave_id) {
    if (!slave_valid(slave_id)) {
        return FAULT_NONE;
    }
    return s_mode[slave_id - 1U];
}

void fault_tick(uint32_t now_ms) {
    for (uint8_t i = 0; i < FAULT_SLAVE_MAX; ++i) {
        if (s_mode[i] == FAULT_NO_RESPONSE &&
            (uint32_t)(now_ms - s_set_ms[i]) >= FAULT_NO_RESPONSE_TIMEOUT_MS) {
            s_mode[i] = FAULT_NONE;
        }
    }
}

bool fault_drop_response(uint8_t slave_id) {
    return fault_get(slave_id) == FAULT_NO_RESPONSE;
}

bool fault_bad_crc(uint8_t slave_id) {
    return fault_get(slave_id) == FAULT_BAD_CRC;
}

bool fault_truncate(uint8_t slave_id) {
    return fault_get(slave_id) == FAULT_TRUNCATED_FRAME;
}

bool fault_force_exception(uint8_t slave_id) {
    return fault_get(slave_id) == FAULT_EXCEPTION_CODE;
}

uint32_t fault_delay_ms(uint8_t slave_id) {
    return (fault_get(slave_id) == FAULT_DELAY_RESPONSE) ? FAULT_DELAY_DEFAULT_MS : 0U;
}
