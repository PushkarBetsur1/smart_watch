#pragma once

#include <stdint.h>

void step_counter_init();
void step_counter_update(float accel_magnitude);
uint32_t step_counter_get_count();
void step_counter_reset();
uint16_t step_counter_get_cadence();
uint32_t step_counter_get_last_step_time();
