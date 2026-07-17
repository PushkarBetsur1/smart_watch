#pragma once

#include <stdint.h>

enum PowerState {
    POWER_ACTIVE,
    POWER_IDLE,
    POWER_SLEEP
};

bool power_init();
void power_wake();
PowerState power_get_state();
void power_set_state(PowerState state);
void power_reset_activity_timer();
uint32_t power_get_idle_time_ms();
