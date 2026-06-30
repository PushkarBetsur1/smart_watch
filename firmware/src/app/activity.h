#pragma once

#include <stdint.h>

enum ActivityType : uint8_t {
    ACTIVITY_IDLE = 0,
    ACTIVITY_WALKING = 1,
    ACTIVITY_RUNNING = 2
};

void activity_init();
void activity_update(uint16_t cadence, float accel_magnitude);
ActivityType activity_get_current();
const char* activity_get_name(ActivityType type);
