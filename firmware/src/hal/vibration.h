#pragma once

#include <stdint.h>

bool vibration_init();
void vibration_pulse(uint16_t duration_ms);
void vibration_pattern(const uint16_t *on_off_ms, uint8_t count);
void vibration_stop();
