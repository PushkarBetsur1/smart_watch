#include "activity.h"
#include "../config.h"
#include <Arduino.h>

static ActivityType current_activity = ACTIVITY_IDLE;
static uint8_t walk_confidence = 0;
static uint8_t run_confidence = 0;

#define CONFIDENCE_THRESHOLD  5
#define CONFIDENCE_MAX        10

void activity_init() {
    current_activity = ACTIVITY_IDLE;
    walk_confidence = 0;
    run_confidence = 0;
}

void activity_update(uint16_t cadence, float accel_magnitude) {
    (void)accel_magnitude;

    if (cadence >= ACTIVITY_RUN_CADENCE_MIN && cadence <= ACTIVITY_RUN_CADENCE_MAX) {
        if (run_confidence < CONFIDENCE_MAX) run_confidence++;
        if (walk_confidence > 0) walk_confidence--;
    } else if (cadence >= ACTIVITY_WALK_CADENCE_MIN && cadence <= ACTIVITY_WALK_CADENCE_MAX) {
        if (walk_confidence < CONFIDENCE_MAX) walk_confidence++;
        if (run_confidence > 0) run_confidence--;
    } else {
        if (walk_confidence > 0) walk_confidence--;
        if (run_confidence > 0) run_confidence--;
    }

    if (run_confidence >= CONFIDENCE_THRESHOLD) {
        current_activity = ACTIVITY_RUNNING;
    } else if (walk_confidence >= CONFIDENCE_THRESHOLD) {
        current_activity = ACTIVITY_WALKING;
    } else {
        current_activity = ACTIVITY_IDLE;
    }
}

ActivityType activity_get_current() {
    return current_activity;
}

const char* activity_get_name(ActivityType type) {
    switch (type) {
        case ACTIVITY_WALKING: return "Walking";
        case ACTIVITY_RUNNING: return "Running";
        default: return "Idle";
    }
}
