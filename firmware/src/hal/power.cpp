#include "power.h"
#include "../config.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

static PowerState current_state = POWER_ACTIVE;
static uint32_t last_activity_ms = 0;

bool power_init() {
    last_activity_ms = millis();
    current_state = POWER_ACTIVE;
    return true;
}

void power_enter_sleep() {
    current_state = POWER_SLEEP;
    __WFE();
    __SEV();
    __WFE();
}

void power_wake() {
    current_state = POWER_ACTIVE;
    last_activity_ms = millis();
}

PowerState power_get_state() {
    return current_state;
}

void power_set_state(PowerState state) {
    current_state = state;
    if (state == POWER_ACTIVE) {
        last_activity_ms = millis();
    }
}

void power_reset_activity_timer() {
    last_activity_ms = millis();
}

uint32_t power_get_idle_time_ms() {
    return millis() - last_activity_ms;
}
