#pragma once

#include <stdint.h>

bool battery_init();
uint16_t battery_read_mv();
uint8_t battery_read_percent();
bool battery_is_charging();
