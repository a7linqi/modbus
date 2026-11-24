#include "app_f103.h"

#include "fault_injection.h"
#include "mb_regs.h"
#include "mb_router.h"
#include "regs_gen.h"
#include "rs485_port.h"

#define APP_SENSOR_PERIOD_MS 200U

static uint32_t s_last_sensor_ms;
static uint16_t s_run_counter;
static uint32_t s_energy;

static void update_simulated_inputs(uint32_t now_ms) {
    if ((uint32_t)(now_ms - s_last_sensor_ms) < APP_SENSOR_PERIOD_MS) {
        return;
    }
    s_last_sensor_ms = now_ms;

    uint16_t tick = (uint16_t)(now_ms / APP_SENSOR_PERIOD_MS);

    /* Slave1：温度 25.0–29.9°C，压力 101.00–101.49kPa，超 29.0°C 报警。 */
    uint16_t temp_raw = (uint16_t)(250U + (tick % 50U));
    uint16_t pressure_raw = (uint16_t)(10100U + (tick % 50U));
    (void)mb_regs_set_input(1, MB_REG_S1_INPUT_TEMPERATURE_ADDR, temp_raw);
    (void)mb_regs_set_input(1, MB_REG_S1_INPUT_PRESSURE_ADDR, pressure_raw);
    (void)mb_regs_set_input(1, MB_REG_S1_INPUT_ALARM_STATUS_ADDR, (temp_raw >= 290U) ? 1U : 0U);

    /* Slave2：电压 220.0V，电流 1.00A，电量累加。 */
    (void)mb_regs_set_input(2, MB_REG_S2_INPUT_VOLTAGE_ADDR, 2200U);
    (void)mb_regs_set_input(2, MB_REG_S2_INPUT_CURRENT_ADDR, 100U);
    s_energy++;
    (void)mb_regs_set_input_u32(2, MB_REG_S2_INPUT_ENERGY_ADDR, s_energy);

    /* Slave3：输出状态跟随输出控制。 */
    uint16_t output_control = 0U;
    if (mb_regs_get_holding(3, MB_REG_S3_HOLDING_OUTPUT_CONTROL_ADDR, &output_control)) {
        (void)mb_regs_set_input(3, MB_REG_S3_INPUT_OUTPUT_STATUS_ADDR, output_control);
    }

    /* Slave4：运行计数。 */
    s_run_counter++;
    (void)mb_regs_set_input(4, MB_REG_S4_INPUT_RUN_COUNTER_ADDR, s_run_counter);
}

void app_init(uint32_t now_ms) {
    mb_regs_init();
    fault_init(now_ms);
    rs485_port_init();
    mb_router_init();

    s_last_sensor_ms = now_ms;
    s_run_counter = 0U;
    s_energy = 0U;
    update_simulated_inputs(now_ms + APP_SENSOR_PERIOD_MS);
}

void app_poll(uint32_t now_ms) {
    rs485_port_tick();
    mb_router_poll(now_ms);
    fault_tick(now_ms);
    update_simulated_inputs(now_ms);
}
