#include "workout.h"
#include "../config.h"
#include <Arduino.h>

static WorkoutSession session;
static void (*stop_callback)(const WorkoutSession &session) = nullptr;

void workout_init() {
    memset(&session, 0, sizeof(WorkoutSession));
    session.state = WORKOUT_STOPPED;
}

void workout_start(uint32_t current_steps) {
    session.state = WORKOUT_ACTIVE;
    session.start_time_ms = millis();
    session.elapsed_ms = 0;
    session.pause_start_ms = 0;
    session.steps_at_start = current_steps;
    session.current_steps = 0;
    session.distance_m = 0;
    session.current_pace_sec_per_km = 0;
    session.average_pace_sec_per_km = 0;
    session.gps_accuracy_m = 0;
    session.gps_last_update_ms = 0;
    session.calories = 0;
    session.activity = ACTIVITY_IDLE;
    session.distance_source = DISTANCE_ESTIMATED;
}

void workout_pause() {
    if (session.state == WORKOUT_ACTIVE) {
        session.state = WORKOUT_PAUSED;
        session.pause_start_ms = millis();
    }
}

void workout_resume() {
    if (session.state == WORKOUT_PAUSED) {
        session.state = WORKOUT_ACTIVE;
        uint32_t paused_duration = millis() - session.pause_start_ms;
        session.start_time_ms += paused_duration;
        session.pause_start_ms = 0;
    }
}

void workout_stop() {
    session.state = WORKOUT_STOPPED;
    if (stop_callback) {
        stop_callback(session);
    }
}

void workout_set_stop_callback(void (*callback)(const WorkoutSession &session)) {
    stop_callback = callback;
}

void workout_update(uint32_t total_steps, ActivityType activity, uint8_t stride_cm, uint8_t weight_kg) {
    if (session.state != WORKOUT_ACTIVE) return;

    session.elapsed_ms = millis() - session.start_time_ms;
    session.current_steps = total_steps - session.steps_at_start;
    session.activity = activity;

    if (session.distance_source == DISTANCE_ESTIMATED) {
        session.distance_m = (session.current_steps * stride_cm) / 100;
    }

    if (session.distance_m > 0) {
        session.average_pace_sec_per_km = (uint16_t)(session.elapsed_ms / session.distance_m);
    }

    // Calories: rough estimate based on MET values
    // Walking ~3.5 MET, Running ~8 MET
    float met = 1.0f;
    if (activity == ACTIVITY_WALKING) met = 3.5f;
    else if (activity == ACTIVITY_RUNNING) met = 8.0f;

    float hours = session.elapsed_ms / 3600000.0f;
    session.calories = (uint16_t)(met * weight_kg * hours);
}

void workout_update_phone_gps(uint32_t distance_m, uint16_t speed_cm_per_sec, uint16_t accuracy_m) {
    if (session.state != WORKOUT_ACTIVE) return;

    session.distance_source = DISTANCE_PHONE_GPS;
    session.distance_m = distance_m;
    session.gps_accuracy_m = accuracy_m;
    session.gps_last_update_ms = millis();
    session.current_pace_sec_per_km = speed_cm_per_sec > 0
        ? (uint16_t)(100000UL / speed_cm_per_sec)
        : 0;

    if (distance_m > 0) {
        session.average_pace_sec_per_km = (uint16_t)((millis() - session.start_time_ms) / distance_m);
    }
}

WorkoutState workout_get_state() {
    return session.state;
}

WorkoutSession workout_get_session() {
    return session;
}

uint32_t workout_get_elapsed_sec() {
    if (session.state == WORKOUT_ACTIVE) {
        return (millis() - session.start_time_ms) / 1000;
    }
    return session.elapsed_ms / 1000;
}

uint32_t workout_get_steps() {
    return session.current_steps;
}

uint32_t workout_get_distance_m() {
    return session.distance_m;
}

uint16_t workout_get_current_pace_sec_per_km() {
    return session.current_pace_sec_per_km;
}

uint16_t workout_get_average_pace_sec_per_km() {
    return session.average_pace_sec_per_km;
}

uint16_t workout_get_gps_accuracy_m() {
    return session.gps_accuracy_m;
}

DistanceSource workout_get_distance_source() {
    return session.distance_source;
}

bool workout_has_fresh_phone_gps() {
    return session.distance_source == DISTANCE_PHONE_GPS &&
           millis() - session.gps_last_update_ms <= 5000;
}

uint16_t workout_get_calories() {
    return session.calories;
}
