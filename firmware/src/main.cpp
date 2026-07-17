#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#include "config.h"
#include "hal/imu.h"
#include "hal/display.h"
#include "hal/battery.h"
#include "hal/ble_service.h"
#include "hal/storage.h"
#include "hal/vibration.h"
#include "hal/power.h"
#include "app/datetime.h"
#include "app/step_counter.h"
#include "app/activity.h"
#include "app/workout.h"
#include "app/ui.h"

// --- Task Handles ---
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t ui_task_handle = NULL;
static TaskHandle_t ble_task_handle = NULL;
static TaskHandle_t power_task_handle = NULL;

// --- Timer Handle ---
static TimerHandle_t second_timer_handle = NULL;

// --- Shared State ---
static UserSettings user_settings;
static volatile bool button_pressed = false;
static volatile uint32_t button_press_time = 0;

// --- Button ISR ---
static void button_isr() {
    button_pressed = true;
    button_press_time = millis();
}

// --- 1-second timer callback (datetime tick) ---
static void second_timer_callback(TimerHandle_t timer) {
    (void)timer;
    datetime_tick();
}

// --- BLE time sync callback ---
static void on_time_sync(const DateTime &dt) {
    datetime_set(dt);
    uint32_t epoch = datetime_to_epoch(dt);
    storage_save_datetime(epoch);
}

static void on_phone_gps(const PhoneGpsUpdate &update) {
    if (!update.has_fix) return;

    workout_update_phone_gps(
        (update.distance_cm + 50) / 100,
        update.speed_cm_per_sec,
        (update.accuracy_cm + 50) / 100
    );
}

static void on_workout_stopped(const WorkoutSession &session) {
    WorkoutRecord record;
    record.timestamp = datetime_is_valid() ? datetime_to_epoch(datetime_get()) : 0;
    record.duration_sec = session.elapsed_ms / 1000;
    record.steps = session.current_steps;
    record.distance_m = session.distance_m;
    record.calories = session.calories;
    record.activity_type = (uint8_t)session.activity;
    storage_save_workout(record);
}

// --- Sensor Task (50 Hz) ---
static void sensor_task(void *param) {
    (void)param;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000 / IMU_SAMPLE_RATE_HZ);

    while (true) {
        ImuData imu_data;
        if (imu_read(imu_data)) {
            float magnitude = imu_accel_magnitude(imu_data);
            step_counter_update(magnitude);

            uint16_t cadence = step_counter_get_cadence();
            activity_update(cadence, magnitude);

            if (workout_get_state() == WORKOUT_ACTIVE) {
                workout_update(
                    step_counter_get_count(),
                    activity_get_current(),
                    user_settings.stride_cm,
                    user_settings.weight_kg
                );
            }
        }

        vTaskDelayUntil(&last_wake, period);
    }
}

// --- UI Task (10 Hz) ---
static void ui_task(void *param) {
    (void)param;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(100);

    while (true) {
        // Handle button
        if (button_pressed) {
            button_pressed = false;
            uint32_t hold_start = button_press_time;

            // Wait for release or long press
            vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));

            if (digitalRead(PIN_BUTTON) == LOW) {
                // Still pressed — wait for release or long press timeout
                while (digitalRead(PIN_BUTTON) == LOW &&
                       millis() - hold_start < BUTTON_VERY_LONG_PRESS_MS) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                uint32_t held_ms = millis() - hold_start;
                if (held_ms >= BUTTON_VERY_LONG_PRESS_MS) {
                    ui_handle_button(BUTTON_VERY_LONG_PRESS);
                    vibration_pulse(250);
                } else if (held_ms >= BUTTON_LONG_PRESS_MS) {
                    ui_handle_button(BUTTON_LONG_PRESS);
                    vibration_pulse(100);
                } else {
                    ui_handle_button(BUTTON_SHORT_PRESS);
                    vibration_pulse(30);
                }
            }
        }

        ui_check_timeout();
        ui_update();

        vTaskDelayUntil(&last_wake, period);
    }
}

// --- BLE Task ---
static void ble_task(void *param) {
    (void)param;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000);

    while (true) {
        if (ble_is_connected()) {
            ble_update_steps(step_counter_get_count());
            ble_update_battery(battery_read_percent());

            BleWorkoutData wd;
            wd.version = 1;
            wd.state = (uint8_t)workout_get_state();
            wd.flags = workout_has_fresh_phone_gps() ? 0x01 : 0x00;
            wd.activity = (uint8_t)activity_get_current();
            wd.steps = workout_get_steps();
            wd.distance_m = workout_get_distance_m();
            wd.elapsed_sec = workout_get_elapsed_sec();
            wd.current_pace_sec_per_km = workout_get_current_pace_sec_per_km();
            wd.average_pace_sec_per_km = workout_get_average_pace_sec_per_km();
            ble_update_workout(wd);
        }

        vTaskDelayUntil(&last_wake, period);
    }
}

// --- Power Task (1 Hz) ---
static void power_task(void *param) {
    (void)param;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000);

    uint32_t last_save_sec = 0;

    while (true) {
        uint32_t idle_ms = power_get_idle_time_ms();

        if (idle_ms > SLEEP_TIMEOUT_MS && workout_get_state() == WORKOUT_STOPPED &&
            power_get_state() != POWER_IDLE) {
            power_set_state(POWER_IDLE);

            DailySteps daily;
            DateTime dt = datetime_get();
            daily.year = dt.year;
            daily.month = dt.month;
            daily.day = dt.day;
            daily.steps = step_counter_get_count();
            daily.distance_m = (daily.steps * user_settings.stride_cm) / 100;
            daily.calories = 0;
            storage_save_daily_steps(daily);
            if (datetime_is_valid()) {
                storage_save_datetime(datetime_to_epoch(dt));
            }
        }

        // Periodic save every 5 minutes
        uint32_t now_sec = millis() / 1000;
        if (now_sec - last_save_sec >= 300) {
            last_save_sec = now_sec;
            DateTime dt = datetime_get();
            if (datetime_is_valid()) {
                storage_save_datetime(datetime_to_epoch(dt));
            }

            DailySteps daily;
            daily.year = dt.year;
            daily.month = dt.month;
            daily.day = dt.day;
            daily.steps = step_counter_get_count();
            daily.distance_m = (daily.steps * user_settings.stride_cm) / 100;
            daily.calories = 0;
            storage_save_daily_steps(daily);
        }

        // Low battery warning
        if (battery_read_percent() <= 10) {
            static uint32_t last_warn = 0;
            if (millis() - last_warn > 60000) {
                vibration_pulse(200);
                last_warn = millis();
            }
        }

        vTaskDelayUntil(&last_wake, period);
    }
}

// --- Arduino Setup ---
void setup() {
    Serial.begin(115200);

    // Initialize HAL
    storage_init();
    display_init();
    battery_init();
    vibration_init();
    power_init();

    // Load settings
    storage_load_settings(user_settings);

    // No battery-backed RTC: time is invalid after every power cycle until phone sync.
    datetime_init();

    // Initialize IMU
    if (!imu_init()) {
        // IMU init failed — show error
        display_clear();
        display_draw_centered_text(28, "IMU ERROR", 2);
        display_update();
        while (true) { delay(1000); }
    }

    // Initialize app modules
    step_counter_init();
    activity_init();
    workout_init();
    workout_set_stop_callback(on_workout_stopped);
    ui_init();

    // Initialize BLE
    ble_init();
    ble_set_time_sync_callback(on_time_sync);
    ble_set_phone_gps_callback(on_phone_gps);
    ble_start_advertising();

    // Button interrupt
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), button_isr, FALLING);

    // 1-second timer for datetime
    second_timer_handle = xTimerCreate("SecTick", pdMS_TO_TICKS(1000), pdTRUE, NULL, second_timer_callback);
    xTimerStart(second_timer_handle, 0);

    // Boot vibration
    vibration_pulse(100);

    // Show splash
    display_clear();
    display_draw_centered_text(20, "SmartWatch", 2);
    display_draw_centered_text(44, "v1.0", 1);
    display_update();
    delay(1500);

    // Create FreeRTOS tasks
    xTaskCreate(sensor_task, "Sensor", TASK_SENSOR_STACK, NULL, TASK_SENSOR_PRIORITY, &sensor_task_handle);
    xTaskCreate(ui_task, "UI", TASK_UI_STACK, NULL, TASK_UI_PRIORITY, &ui_task_handle);
    xTaskCreate(ble_task, "BLE", TASK_BLE_STACK, NULL, TASK_BLE_PRIORITY, &ble_task_handle);
    xTaskCreate(power_task, "Power", TASK_POWER_STACK, NULL, TASK_POWER_PRIORITY, &power_task_handle);
}

// Arduino loop is unused — FreeRTOS scheduler runs all tasks
void loop() {
    vTaskDelay(portMAX_DELAY);
}
