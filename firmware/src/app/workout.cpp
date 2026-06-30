#include "workout.h"
#include "../config.h"
#include <Arduino.h>

static WorkoutSession session;

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
    session.calories = 0;
    session.activity = ACTIVITY_IDLE;
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
}

void workout_update(uint32_t total_steps, ActivityType activity, uint8_t stride_cm, uint8_t weight_kg) {
    if (session.state != WORKOUT_ACTIVE) return;

    session.elapsed_ms = millis() - session.start_time_ms;
    session.current_steps = total_steps - session.steps_at_start;
    session.activity = activity;

    // Distance = steps * stride
    session.distance_m = (session.current_steps * stride_cm) / 100;

    // Calories: rough estimate based on MET values
    // Walking ~3.5 MET, Running ~8 MET
    float met = 1.0f;
    if (activity == ACTIVITY_WALKING) met = 3.5f;
    else if (activity == ACTIVITY_RUNNING) met = 8.0f;

    float hours = session.elapsed_ms / 3600000.0f;
    session.calories = (uint16_t)(met * weight_kg * hours);
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

uint16_t workout_get_calories() {
    return session.calories;
}
