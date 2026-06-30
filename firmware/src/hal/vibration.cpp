#include "vibration.h"
#include "../config.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

bool vibration_init() {
    pinMode(PIN_VIBRATION, OUTPUT);
    digitalWrite(PIN_VIBRATION, LOW);
    return true;
}

void vibration_pulse(uint16_t duration_ms) {
    digitalWrite(PIN_VIBRATION, HIGH);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    digitalWrite(PIN_VIBRATION, LOW);
}

void vibration_pattern(const uint16_t *on_off_ms, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        if (i % 2 == 0) {
            digitalWrite(PIN_VIBRATION, HIGH);
        } else {
            digitalWrite(PIN_VIBRATION, LOW);
        }
        vTaskDelay(pdMS_TO_TICKS(on_off_ms[i]));
    }
    digitalWrite(PIN_VIBRATION, LOW);
}

void vibration_stop() {
    digitalWrite(PIN_VIBRATION, LOW);
}
