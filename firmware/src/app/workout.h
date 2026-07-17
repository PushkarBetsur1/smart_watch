#pragma once

#include <stdint.h>
#include "activity.h"

enum WorkoutState : uint8_t {
    WORKOUT_STOPPED = 0,
    WORKOUT_ACTIVE = 1,
    WORKOUT_PAUSED = 2
};

enum DistanceSource : uint8_t {
    DISTANCE_ESTIMATED = 0,
    DISTANCE_PHONE_GPS = 1
};

struct WorkoutSession {
    WorkoutState state;
    uint32_t start_time_ms;
    uint32_t elapsed_ms;
    uint32_t pause_start_ms;
    uint32_t steps_at_start;
    uint32_t current_steps;
    uint32_t distance_m;
    uint16_t current_pace_sec_per_km;
    uint16_t average_pace_sec_per_km;
    uint16_t gps_accuracy_m;
    uint32_t gps_last_update_ms;
    uint16_t calories;
    ActivityType activity;
    DistanceSource distance_source;
};

void workout_init();
void workout_start(uint32_t current_steps);
void workout_pause();
void workout_resume();
void workout_stop();
void workout_set_stop_callback(void (*callback)(const WorkoutSession &session));
void workout_update(uint32_t total_steps, ActivityType activity, uint8_t stride_cm, uint8_t weight_kg);
void workout_update_phone_gps(uint32_t distance_m, uint16_t speed_cm_per_sec, uint16_t accuracy_m);
WorkoutState workout_get_state();
WorkoutSession workout_get_session();
uint32_t workout_get_elapsed_sec();
uint32_t workout_get_steps();
uint32_t workout_get_distance_m();
uint16_t workout_get_current_pace_sec_per_km();
uint16_t workout_get_average_pace_sec_per_km();
uint16_t workout_get_gps_accuracy_m();
DistanceSource workout_get_distance_source();
bool workout_has_fresh_phone_gps();
uint16_t workout_get_calories();
