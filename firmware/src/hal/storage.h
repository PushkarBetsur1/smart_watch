#pragma once

#include <stdint.h>

struct UserSettings {
    uint8_t  stride_cm;
    uint8_t  weight_kg;
    uint8_t  height_cm;
    uint8_t  screen_brightness;
    uint16_t screen_timeout_ms;
};

struct DailySteps {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint32_t steps;
    uint32_t distance_m;
    uint16_t calories;
};

struct WorkoutRecord {
    uint32_t timestamp;
    uint32_t duration_sec;
    uint32_t steps;
    uint32_t distance_m;
    uint16_t calories;
    uint8_t  activity_type;
};

bool storage_init();
bool storage_save_settings(const UserSettings &settings);
bool storage_load_settings(UserSettings &settings);
bool storage_save_daily_steps(const DailySteps &steps);
bool storage_load_daily_steps(DailySteps &steps);
bool storage_save_workout(const WorkoutRecord &record);
uint8_t storage_load_workouts(WorkoutRecord *records, uint8_t max_count);
bool storage_save_datetime(uint32_t epoch);
uint32_t storage_load_datetime();
bool storage_format();
