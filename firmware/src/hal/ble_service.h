#pragma once

#include <stdint.h>
#include "../app/datetime.h"

struct BleWorkoutData {
    uint32_t steps;
    uint32_t distance_m;
    uint32_t elapsed_sec;
    uint16_t calories;
    uint8_t  activity;
};

bool ble_init();
void ble_start_advertising();
void ble_stop_advertising();
bool ble_is_connected();
void ble_update_steps(uint32_t steps);
void ble_update_battery(uint8_t percent);
void ble_update_workout(const BleWorkoutData &data);
void ble_set_time_sync_callback(void (*callback)(const DateTime &dt));
