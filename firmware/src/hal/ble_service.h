#pragma once

#include <stdint.h>
#include "../app/datetime.h"

struct __attribute__((packed)) BleWorkoutData {
    uint8_t  version;
    uint8_t  state;
    uint8_t  flags;
    uint8_t  activity;
    uint32_t steps;
    uint32_t distance_m;
    uint32_t elapsed_sec;
    uint16_t current_pace_sec_per_km;
    uint16_t average_pace_sec_per_km;
};

struct PhoneGpsUpdate {
    bool     has_fix;
    uint32_t distance_cm;
    uint16_t speed_cm_per_sec;
    uint16_t accuracy_cm;
};

bool ble_init();
void ble_start_advertising();
void ble_stop_advertising();
bool ble_is_connected();
void ble_update_steps(uint32_t steps);
void ble_update_battery(uint8_t percent);
void ble_update_workout(const BleWorkoutData &data);
void ble_set_time_sync_callback(void (*callback)(const DateTime &dt));
void ble_set_phone_gps_callback(void (*callback)(const PhoneGpsUpdate &update));
