#pragma once

#include <Arduino.h>

// --- Pin Definitions (Seeed XIAO nRF52840) ---

// OLED Display (SPI)
#define PIN_OLED_CS     7
#define PIN_OLED_DC     2
#define PIN_OLED_RST    3

// Button
#define PIN_BUTTON      0

// Vibration Motor
#define PIN_VIBRATION   6

// Battery (XIAO Sense built-in switched divider)
#define PIN_BATTERY         PIN_VBAT
#define PIN_BATTERY_ENABLE  VBAT_ENABLE
#define PIN_CHARGE_STATUS   23

// --- Display Config ---
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

// --- IMU Config ---
#define IMU_SAMPLE_RATE_HZ  52
#define IMU_ACCEL_RANGE     4   // +/- 4g

// --- Step Counter Config ---
#define STEP_THRESHOLD_LOW      0.8f
#define STEP_THRESHOLD_HIGH     1.8f
#define STEP_MIN_INTERVAL_MS    250
#define STEP_MAX_INTERVAL_MS    2000

// --- Activity Detection ---
#define ACTIVITY_WALK_CADENCE_MIN   70
#define ACTIVITY_WALK_CADENCE_MAX   140
#define ACTIVITY_RUN_CADENCE_MIN    140
#define ACTIVITY_RUN_CADENCE_MAX    220

// --- Power Config ---
#define BATTERY_FULL_MV         4200
#define BATTERY_EMPTY_MV        3300
#define SCREEN_TIMEOUT_MS       5000
#define SLEEP_TIMEOUT_MS        30000

// --- FreeRTOS Task Config ---
#define TASK_SENSOR_STACK       512
#define TASK_UI_STACK           1024
#define TASK_BLE_STACK          512
#define TASK_POWER_STACK        256

#define TASK_SENSOR_PRIORITY    3
#define TASK_UI_PRIORITY        2
#define TASK_BLE_PRIORITY       2
#define TASK_POWER_PRIORITY     1

// --- BLE Config ---
#define DEVICE_NAME             "SmartWatch"
#define BLE_TX_POWER            0   // dBm

// --- Storage Config ---
#define MAX_WORKOUT_HISTORY     20
#define STORAGE_FILENAME        "/settings.bin"
#define WORKOUT_FILENAME        "/workouts.bin"
#define STEPS_FILENAME          "/steps.bin"

// --- User Defaults ---
#define DEFAULT_STRIDE_CM       75
#define DEFAULT_WEIGHT_KG       70
#define DEFAULT_HEIGHT_CM       175

// --- Button Timing ---
#define BUTTON_DEBOUNCE_MS      50
#define BUTTON_LONG_PRESS_MS    1000
#define BUTTON_VERY_LONG_PRESS_MS 3000
