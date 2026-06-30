#include "step_counter.h"
#include "../config.h"
#include <Arduino.h>

static uint32_t step_count = 0;
static uint32_t last_step_ms = 0;
static uint16_t cadence_bpm = 0;

static float filter_buf[4] = {0};
static uint8_t filter_idx = 0;

static bool was_above_threshold = false;
static float adaptive_threshold = 1.2f;
static float peak_val = 0.0f;
static float valley_val = 2.0f;

static float low_pass_filter(float input) {
    filter_buf[filter_idx] = input;
    filter_idx = (filter_idx + 1) % 4;
    float sum = 0;
    for (uint8_t i = 0; i < 4; i++) {
        sum += filter_buf[i];
    }
    return sum / 4.0f;
}

void step_counter_init() {
    step_count = 0;
    last_step_ms = 0;
    cadence_bpm = 0;
    was_above_threshold = false;
    adaptive_threshold = 1.2f;
    peak_val = 0.0f;
    valley_val = 2.0f;
    filter_idx = 0;
    for (uint8_t i = 0; i < 4; i++) filter_buf[i] = 0;
}

void step_counter_update(float accel_magnitude) {
    float filtered = low_pass_filter(accel_magnitude);

    if (filtered > peak_val) peak_val = filtered;
    if (filtered < valley_val) valley_val = filtered;

    // Slowly decay peak/valley toward center
    peak_val *= 0.999f;
    valley_val = valley_val * 0.999f + 0.001f;

    adaptive_threshold = (peak_val + valley_val) / 2.0f;

    if (adaptive_threshold < STEP_THRESHOLD_LOW) {
        adaptive_threshold = STEP_THRESHOLD_LOW;
    }
    if (adaptive_threshold > STEP_THRESHOLD_HIGH) {
        adaptive_threshold = STEP_THRESHOLD_HIGH;
    }

    bool above = filtered > adaptive_threshold;

    if (above && !was_above_threshold) {
        uint32_t now = millis();
        uint32_t interval = now - last_step_ms;

        if (interval >= STEP_MIN_INTERVAL_MS && interval <= STEP_MAX_INTERVAL_MS) {
            step_count++;
            cadence_bpm = (uint16_t)(60000UL / interval);
            last_step_ms = now;
        } else if (interval > STEP_MAX_INTERVAL_MS) {
            last_step_ms = now;
        }
    }

    was_above_threshold = above;
}

uint32_t step_counter_get_count() {
    return step_count;
}

void step_counter_reset() {
    step_count = 0;
    cadence_bpm = 0;
}

uint16_t step_counter_get_cadence() {
    if (millis() - last_step_ms > 3000) {
        cadence_bpm = 0;
    }
    return cadence_bpm;
}

uint32_t step_counter_get_last_step_time() {
    return last_step_ms;
}
