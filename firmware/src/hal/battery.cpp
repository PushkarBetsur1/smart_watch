#include "battery.h"
#include "../config.h"
#include <Arduino.h>

static const uint16_t VBAT_DIVIDER_FACTOR = 2;

bool battery_init() {
    analogReadResolution(12);
    pinMode(PIN_BATTERY, INPUT);
    pinMode(PIN_BATTERY_ENABLE, OUTPUT);
    digitalWrite(PIN_BATTERY_ENABLE, HIGH);
    pinMode(PIN_CHARGE_STATUS, INPUT_PULLUP);
    return true;
}

uint16_t battery_read_mv() {
    digitalWrite(PIN_BATTERY_ENABLE, LOW);
    delayMicroseconds(100);
    uint32_t raw = analogRead(PIN_BATTERY);
    digitalWrite(PIN_BATTERY_ENABLE, HIGH);
    // XIAO nRF52840: 3.3V reference, 12-bit ADC, voltage divider /2
    uint16_t mv = (raw * 3300 * VBAT_DIVIDER_FACTOR) / 4095;
    return mv;
}

uint8_t battery_read_percent() {
    uint16_t mv = battery_read_mv();
    if (mv >= BATTERY_FULL_MV) return 100;
    if (mv <= BATTERY_EMPTY_MV) return 0;
    return (uint8_t)((uint32_t)(mv - BATTERY_EMPTY_MV) * 100 / (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
}

bool battery_is_charging() {
    return digitalRead(PIN_CHARGE_STATUS) == LOW;
}
