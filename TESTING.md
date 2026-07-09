# Testing Guide

## Overview

This document describes how to test each module of the smartwatch firmware, from individual hardware verification to full system integration tests. Testing is performed on the Seeed XIAO nRF52840 development board with connected peripherals.

Tests are split into two categories:
- **Unit Tests (1–28):** Hardware-isolated, repeatable on the bench. Can be re-run cheaply after any code change.
- **Field Tests (29–32):** Require a human wearing the device and physically walking/running. Run these when validating a firmware release, not on every change.

---

## Hardware Setup for Testing

### Required Components

| Component | Connection | Notes |
|-----------|-----------|-------|
| Seeed XIAO nRF52840 | USB-C to PC | Serial monitor at 115200 baud |
| BMI270 breakout | I2C (SDA→D4, SCL→D5, INT1→D1) | 3.3V supply |
| 1.3" SSD1306 OLED | SPI (SCK→D8, MOSI→D10, CS→D7, DC→D2, RST→D3) | 3.3V supply |
| Tactile button | D0 → GND | Uses internal pull-up |
| Coin vibration motor | D6 → MOSFET gate | Flyback diode required |
| LiPo battery | JST connector | 3.7V, 300-500mAh |

### Wiring Verification Checklist

- [ ] I2C pull-ups present (4.7kΩ to 3.3V on SDA and SCL)
- [ ] OLED powered and SPI wired correctly (check DC/CS not swapped)
- [ ] Button connects pin D0 to GND when pressed
- [ ] Motor MOSFET gate on D6, drain to motor, source to GND
- [ ] Battery connected, voltage divider readable on A0

---

## Module Tests

### 1. IMU (BMI270)

**Test: Chip ID Read**

Add to `setup()` temporarily:

```cpp
#include "hal/imu.h"

void setup() {
    Serial.begin(115200);
    delay(2000);

    if (imu_init()) {
        Serial.println("IMU OK - Chip ID 0x24 confirmed");
    } else {
        Serial.println("IMU FAIL - check wiring");
    }
}
```

Expected output: `IMU OK - Chip ID 0x24 confirmed`

**Test: Accelerometer Data**

```cpp
void loop() {
    ImuData data;
    if (imu_read(data)) {
        Serial.printf("X: %.2f  Y: %.2f  Z: %.2f  Mag: %.2f\n",
            data.accel_x, data.accel_y, data.accel_z,
            imu_accel_magnitude(data));
    }
    delay(100);
}
```

Expected:
- At rest flat on table: X≈0, Y≈0, Z≈1.0, Mag≈1.0
- Tilted: X/Y change, Mag stays ≈1.0
- Shaken: Mag spikes above 1.5

**Test: Sleep/Wake**

```cpp
imu_sleep();
delay(1000);
// Verify current draw drops (measure with multimeter: ~3.5µA)
imu_wake();
// Verify readings resume
```

**Pass Criteria:**
- Chip ID reads 0x24
- Accelerometer magnitude ≈1.0g at rest
- Values update at 50Hz
- Sleep current < 5µA

---

### 2. Display (SSD1306)

**Test: Initialization and Text**

```cpp
#include "hal/display.h"

void setup() {
    if (display_init()) {
        display_clear();
        display_draw_centered_text(10, "Hello", 2);
        display_draw_centered_text(40, "SmartWatch", 1);
        display_update();
    }
}
```

Expected: "Hello" in large text centered, "SmartWatch" below it.

**Test: All Drawing Functions**

```cpp
void setup() {
    display_init();
    display_clear();
    display_draw_rect(0, 0, 128, 64);
    display_draw_line(0, 0, 127, 63);
    display_draw_progress_bar(10, 50, 108, 10, 75);
    display_update();
}
```

Expected: Border rectangle, diagonal line, 75% filled progress bar.

**Test: Brightness and On/Off**

```cpp
display_set_brightness(0);    // Dimmest
delay(1000);
display_set_brightness(255);  // Brightest
delay(1000);
display_off();                // Screen blank, no glow
delay(1000);
display_on();                 // Screen returns
```

**Pass Criteria:**
- Text renders correctly at sizes 1, 2, 3
- Centered text is actually centered
- `display_off()` fully blanks the screen
- No flickering or artifacts during updates

---

### 3. Battery

**Test: Voltage Reading**

```cpp
#include "hal/battery.h"

void setup() {
    Serial.begin(115200);
    battery_init();
}

void loop() {
    Serial.printf("mV: %u  Percent: %u%%  Charging: %s\n",
        battery_read_mv(),
        battery_read_percent(),
        battery_is_charging() ? "yes" : "no");
    delay(1000);
}
```

Expected:
- USB powered (no battery): ~5000mV raw (clamped to 100%)
- Full LiPo: 4100-4200mV, 90-100%
- Discharged LiPo: 3300-3600mV, 0-30%

**Test: Charging Detection**

1. Connect battery
2. Connect USB-C
3. Verify `battery_is_charging()` returns true
4. Disconnect USB-C
5. Verify returns false

**Pass Criteria:**
- Voltage reading within ±50mV of multimeter measurement
- Percentage maps linearly between 3300mV (0%) and 4200mV (100%)
- Charging state reflects actual charge IC status

---

### 4. Vibration Motor

**Test: Single Pulse**

```cpp
#include "hal/vibration.h"

void setup() {
    vibration_init();
    vibration_pulse(200);  // 200ms buzz
}
```

Expected: Single noticeable vibration.

**Test: Pattern**

```cpp
uint16_t pattern[] = {100, 100, 100, 100, 300};  // short-short-long
vibration_pattern(pattern, 5);
```

Expected: Two short buzzes followed by one long buzz.

**Pass Criteria:**
- Motor activates on pulse
- Motor stops cleanly (no residual spin)
- `vibration_stop()` immediately kills motor mid-pattern

---

### 5. Storage (LittleFS)

**Test: Settings Save/Load**

```cpp
#include "hal/storage.h"

void setup() {
    Serial.begin(115200);
    storage_init();

    UserSettings save = {75, 70, 175, 128, 5000};
    storage_save_settings(save);

    UserSettings load;
    if (storage_load_settings(load)) {
        Serial.printf("Stride: %u  Weight: %u  Height: %u\n",
            load.stride_cm, load.weight_kg, load.height_cm);
    }
}
```

Expected: Values match what was saved.

**Test: Persistence Across Reboot**

1. Save settings
2. Power cycle the board (unplug and replug USB)
3. Load settings
4. Verify values survived

**Test: Workout History**

```cpp
WorkoutRecord rec = {1000000, 1800, 3000, 2400, 150, 1};
storage_save_workout(rec);
storage_save_workout(rec);  // Save two

WorkoutRecord loaded[5];
uint8_t count = storage_load_workouts(loaded, 5);
Serial.printf("Loaded %u workouts\n", count);
```

Expected: `Loaded 2 workouts`

**Test: Format**

```cpp
storage_format();
UserSettings s;
bool found = storage_load_settings(s);  // Should return false
```

**Pass Criteria:**
- Data persists across power cycles
- Multiple workout records append correctly
- Format clears all data
- Default values returned when no saved data exists

---

### 6. BLE Service

**Test: Advertising**

```cpp
#include "hal/ble_service.h"

void setup() {
    ble_init();
    ble_start_advertising();
    Serial.println("Advertising as 'SmartWatch'");
}
```

Verify with phone:
1. Open nRF Connect app (iOS/Android)
2. Scan for "SmartWatch"
3. Confirm it appears in scan results

**Test: Connection and Characteristics**

1. Connect via nRF Connect
2. Browse services — find custom service UUID `00001000-...`
3. Read steps characteristic — should return 0x00000000
4. Enable notifications on steps
5. Verify notifications arrive when `ble_update_steps()` is called

**Test: Time Sync**

1. Connect via nRF Connect
2. Write 7 bytes to time sync characteristic

```
Byte layout: [year_lo, year_hi, month, day, hour, minute, second]
Example for 2024-06-15 14:30:00:
E8 07 06 0F 0E 1E 00
```

Note: weekday is **not** in the payload — the watch derives it internally from the date.

3. Verify watch updates its displayed time and shows the correct weekday

**Test: Weekday Derivation**

1. Connect via nRF Connect
2. Write a time sync for a date where Monday is the correct weekday (e.g., 2024-07-15 is a Monday)
3. Verify the watch displays "Mon" — regardless of what any previous weekday state was
4. This confirms the watch computes weekday from date rather than trusting the phone

**Test: Disconnect/Reconnect**

1. Connect
2. Walk out of range
3. Verify advertising restarts automatically
4. Walk back in range
5. Reconnect

**Pass Criteria:**
- Device appears in BLE scans within 5 seconds
- All characteristics readable
- Notifications delivered within 1 second of update
- Auto-reconnect advertising after disconnect
- Time sync callback fires correctly

---

### 7. Power Management

**Test: Idle Timer**

```cpp
#include "hal/power.h"

void setup() {
    power_init();
}

void loop() {
    Serial.printf("Idle: %lu ms  State: %u\n",
        power_get_idle_time_ms(), power_get_state());
    delay(1000);
}
```

Expected: Idle time increases every second.

**Test: Activity Reset**

```cpp
power_reset_activity_timer();
// Idle time should reset to 0
```

**Test: Sleep/Wake**

Verify current draw with multimeter:
- Active: ~5-8mA (CPU + peripherals)
- Sleep: < 20µA (target — see SYSTEM_ARCHITECTURE.md power budget deep-sleep row for component breakdown)

**Pass Criteria:**
- Idle timer increments correctly
- `power_reset_activity_timer()` resets to 0
- Sleep mode achieves < 20µA system current

---

## Application Module Tests

### 8. DateTime

**Test: Tick Accuracy**

```cpp
#include "app/datetime.h"

void setup() {
    datetime_init();
    DateTime dt = {2024, 6, 30, 6, 23, 59, 55};  // Sun Jun 30, 23:59:55
    datetime_set(dt);
}

void loop() {
    datetime_tick();
    DateTime dt = datetime_get();
    Serial.printf("%s %s %u %04u  %02u:%02u:%02u\n",
        datetime_weekday_str(dt.weekday),
        datetime_month_str(dt.month),
        dt.day, dt.year,
        dt.hour, dt.minute, dt.second);
    delay(1000);
}
```

Expected after 5 ticks:
```
Sun Jun 30 2024  23:59:55
Sun Jun 30 2024  23:59:56
Sun Jun 30 2024  23:59:57
Sun Jun 30 2024  23:59:58
Sun Jun 30 2024  23:59:59
Mon Jul 1 2024  00:00:00   ← Day, weekday, month roll over
```

**Test: Leap Year**

```cpp
DateTime dt = {2024, 2, 28, 2, 23, 59, 59};  // Wed Feb 28, 2024 (leap year)
datetime_set(dt);
datetime_tick();
DateTime result = datetime_get();
// Should be Feb 29, not Mar 1
assert(result.month == 2 && result.day == 29);

DateTime dt2 = {2023, 2, 28, 1, 23, 59, 59};  // Non-leap year
datetime_set(dt2);
datetime_tick();
DateTime result2 = datetime_get();
// Should be Mar 1
assert(result2.month == 3 && result2.day == 1);
```

**Test: Epoch Conversion Round-trip**

```cpp
DateTime dt = {2024, 7, 15, 0, 12, 30, 45};
uint32_t epoch = datetime_to_epoch(dt);
DateTime back = datetime_from_epoch(epoch);
// All fields should match
```

**Pass Criteria:**
- Seconds/minutes/hours/days roll over correctly
- Leap years handled (2024=leap, 2023=not, 2100=not)
- Weekday advances correctly
- Epoch round-trip preserves all fields

---

### 9. Step Counter

> **Important — scope of these tests:** The simulated sine-wave tests below are *regression tests* — they catch math bugs when the peak-detection algorithm changes. They do **not** validate real-world accuracy. Synthetic data has no arm swing, gait variability, phone vibration, or driving bumps. The real accuracy bar is Field Test #29 (actual walk test). Consider recording real accelerometer logs from walking, running, typing, and driving, then replaying them as fixtures for higher-confidence regression testing.

**Test: Known Step Simulation**

```cpp
#include "app/step_counter.h"
#include <math.h>

void setup() {
    Serial.begin(115200);
    step_counter_init();

    // Simulate 100 steps at 2 Hz (walking cadence ~120 bpm)
    // Each step: sinusoidal peak in acceleration magnitude
    for (int i = 0; i < 5000; i++) {  // 5000 samples at 50Hz = 100 seconds
        float t = i / 50.0f;
        // Walking signal: 1g baseline + 0.4g peak at 2 Hz
        float mag = 1.0f + 0.4f * sinf(2 * M_PI * 2.0f * t);
        step_counter_update(mag);
        delay(1);  // Need millis() to advance for interval checks
    }

    Serial.printf("Steps counted: %lu (expected ~200)\n", step_counter_get_count());
}
```

Expected: Step count between 180-220 (±10% of actual 200 steps in 100s at 2Hz).

**Test: Rejection of Non-Step Motion**

```cpp
step_counter_init();

// Simulate random vibration (not periodic)
for (int i = 0; i < 2500; i++) {
    float mag = 1.0f + 0.1f * (random(100) / 100.0f);
    step_counter_update(mag);
    delay(1);
}

Serial.printf("Noise steps: %lu (should be < 5)\n", step_counter_get_count());
```

Expected: < 5 false steps from random noise.

**Test: Cadence Reporting**

After walking simulation:
```cpp
uint16_t cadence = step_counter_get_cadence();
Serial.printf("Cadence: %u bpm (expected ~120)\n", cadence);
```

**Pass Criteria:**
- ≥95% accuracy on simulated walking (2Hz periodic)
- ≥90% accuracy on simulated running (3Hz periodic)
- < 5 false steps from random noise over 50 seconds
- Cadence within ±10 bpm of actual

---

### 10. Activity Detection

**Test: Walking Detection**

```cpp
#include "app/activity.h"

activity_init();

// Feed walking cadence for 10 updates
for (int i = 0; i < 10; i++) {
    activity_update(110, 1.3f);  // 110 bpm, moderate accel
}

Serial.printf("Activity: %s (expected Walking)\n",
    activity_get_name(activity_get_current()));
```

**Test: Running Detection**

```cpp
for (int i = 0; i < 10; i++) {
    activity_update(160, 2.0f);  // 160 bpm, high accel
}
// Should report Running
```

**Test: Idle Detection**

```cpp
for (int i = 0; i < 10; i++) {
    activity_update(0, 1.0f);  // No cadence
}
// Should report Idle
```

**Test: Transition Hysteresis**

```cpp
// Walking → single idle sample → should still be Walking
for (int i = 0; i < 10; i++) activity_update(110, 1.3f);
activity_update(0, 1.0f);  // One idle sample
// Should still be Walking (confidence hasn't dropped enough)
```

**Pass Criteria:**
- Walking detected within 5 updates at cadence 70-140
- Running detected within 5 updates at cadence 140-220
- Idle detected when cadence drops to 0 for several updates
- No rapid oscillation between states (hysteresis works)

---

### 11. Workout Manager

**Test: Full Workout Lifecycle**

```cpp
#include "app/workout.h"

workout_init();

// Start
workout_start(1000);  // 1000 steps already counted today
assert(workout_get_state() == WORKOUT_ACTIVE);

// Simulate progress
delay(5000);
workout_update(1050, ACTIVITY_WALKING, 75, 70);
assert(workout_get_steps() == 50);
assert(workout_get_distance_m() == 37);  // 50 * 75cm = 37.5m

// Pause
workout_pause();
assert(workout_get_state() == WORKOUT_PAUSED);
uint32_t elapsed_at_pause = workout_get_elapsed_sec();
delay(2000);
assert(workout_get_elapsed_sec() == elapsed_at_pause);  // Timer frozen

// Resume
workout_resume();
assert(workout_get_state() == WORKOUT_ACTIVE);

// Stop
workout_stop();
assert(workout_get_state() == WORKOUT_STOPPED);
```

**Test: Calorie Calculation**

> **Note:** Do not use `delay(3600000)` — a 1-hour blocking delay is not testable. The correct approach is to make `workout.cpp` accept an injectable `now_ms()` function pointer so tests can fast-forward time. Until that refactor is done, test calorie scaling at shorter intervals and verify proportionality.

```cpp
workout_start(0);
// Inject a fake elapsed time of 10 minutes (600 seconds)
// workout_update internally uses millis() — you need now_ms() injection for full testing
// For now: verify calories are non-zero and scale correctly with activity type
workout_update(1000, ACTIVITY_WALKING, 75, 70);
uint16_t walk_cal = workout_get_calories();

workout_init();
workout_start(0);
workout_update(1000, ACTIVITY_RUNNING, 75, 70);
uint16_t run_cal = workout_get_calories();

// Running should give more calories than walking for same steps
assert(run_cal > walk_cal);
Serial.printf("Walk cal: %u  Run cal: %u\n", walk_cal, run_cal);
```

**Pass Criteria:**
- State transitions work (stopped→active→paused→active→stopped)
- Steps = total_steps - steps_at_start
- Distance = steps × stride / 100
- Timer pauses during PAUSED state
- Calories scale with time, weight, and activity

---

### 12. UI

**Test: Screen Cycling**

```cpp
#include "app/ui.h"

ui_init();

assert(ui_get_screen() == SCREEN_HOME);
ui_handle_button(BUTTON_SHORT_PRESS);
assert(ui_get_screen() == SCREEN_WORKOUT);
ui_handle_button(BUTTON_SHORT_PRESS);
assert(ui_get_screen() == SCREEN_STATS);
ui_handle_button(BUTTON_SHORT_PRESS);
assert(ui_get_screen() == SCREEN_HOME);  // Wraps around
```

**Test: Long Press Starts Workout**

```cpp
ui_init();
ui_handle_button(BUTTON_LONG_PRESS);
assert(workout_get_state() == WORKOUT_ACTIVE);
assert(ui_get_screen() == SCREEN_WORKOUT);
```

**Test: Screen Timeout**

```cpp
ui_init();
assert(ui_is_screen_on() == true);
delay(SCREEN_TIMEOUT_MS + 100);
ui_check_timeout();
assert(ui_is_screen_on() == false);
```

**Test: Wake on Button**

```cpp
// Screen is off
ui_handle_button(BUTTON_SHORT_PRESS);
assert(ui_is_screen_on() == true);
// Screen should NOT have changed (first press only wakes)
```

**Pass Criteria:**
- Short press cycles through all 3 screens in order
- Long press toggles workout start/pause/stop
- Screen turns off after timeout
- Button press wakes screen without changing screen
- No crashes on rapid button presses

---

## Unit Tests (1–28)

## Field Tests (29–33)

### 29. Full System — Battery-Dead Date/Time Recovery

**Procedure:**

1. Let battery fully discharge (watch powers off)
2. Recharge and power on
3. Observe home screen before any BLE sync

**Pass Criteria:**
- Watch does NOT silently display a wrong date/time (e.g., Jan 1 2024)
- UI shows a "Time unset — sync phone" indicator or similar stale-time warning
- After BLE sync from phone, time displays correctly and warning clears

---

### 30. Full System — Walk Test

**Procedure:**

1. Flash full firmware
2. Strap to wrist
3. Count exactly 100 steps manually while walking
4. Compare watch step count

**Pass Criteria:**
- Counted steps within 95-105 of actual 100
- Activity shows "Walking" during walk
- Screen displays updating step count

### 31. Full System — Workout Flow

**Procedure:**

1. Long press to start workout
2. Walk/run for 5 minutes
3. Long press to pause
4. Verify timer stops
5. Long press to stop workout
6. Check stats screen for summary

**Pass Criteria:**
- All transitions trigger vibration feedback
- Timer accurate to ±2 seconds over 5 minutes
- Distance reasonable for actual distance walked
- Data persists (power cycle and check stored workouts)

### 32. Full System — BLE Sync

**Procedure:**

1. Start watch, accumulate some steps
2. Connect phone via nRF Connect
3. Read step characteristic
4. Write time sync
5. Verify watch clock updates and weekday is correct
6. Disconnect and verify advertising resumes

**Pass Criteria:**
- Step value matches watch display
- Time syncs within 1 second
- Weekday derived correctly from date
- Reconnection works after disconnect

### 33. Full System — Power/Battery

**Procedure:**

1. Fully charge battery
2. Run watch in normal mode (screen timeout enabled)
3. Check every 12 hours
4. Record battery percentage over time

**Pass Criteria:**
- Battery percentage decreases monotonically
- Watch runs for ≥48 hours (2 days minimum)
- Low battery vibration triggers at 10%
- No crashes or lockups during extended run

---

## Performance Benchmarks

| Metric | Target | How to Measure |
|--------|--------|----------------|
| Sensor task loop time | < 20ms | `millis()` delta in sensor task |
| UI refresh time | < 50ms | `millis()` delta around `ui_update()` |
| BLE notification latency | < 100ms | Timestamp in nRF Connect vs send time |
| Boot to display | < 2s | Stopwatch from power-on to splash screen |
| Step detection latency | < 500ms | Steps appear within 1 step of actual |
| Flash write time | < 50ms | `millis()` around `storage_save_*()` |

---

## Debugging Tips

### Serial Monitor Not Working
- Verify `monitor_speed = 115200` in platformio.ini
- Wait 2 seconds after boot before printing (USB enumeration delay)
- Use `while (!Serial) delay(10);` for critical init messages

### IMU Returns All Zeros
- Check I2C address (0x68 with SDO→GND, 0x69 with SDO→VCC)
- Verify 3.3V supply to BMI270 (not 5V!)
- Check pull-up resistors on SDA/SCL

### Display Blank
- Verify SPI wiring order (MOSI/SCK/CS/DC/RST)
- Check that DC and CS aren't swapped
- Try `display_set_brightness(255)`

### Steps Not Counting
- Print raw accelerometer magnitude to Serial
- Verify magnitude ≈1.0g at rest
- Check that `STEP_THRESHOLD_LOW` / `HIGH` bracket the walking peak amplitude

### BLE Not Visible
- Ensure `ble_start_advertising()` is called after `ble_init()`
- Check that SoftDevice is flashed (comes with Adafruit bootloader)
- Try resetting the board — BLE stack can get stuck

### High Power Consumption
- Verify IMU enters sleep when idle
- Verify display turns off after timeout
- Check for busy-wait loops (use `vTaskDelay` not `delay` in tasks)
- Measure with multimeter in series with battery

---

## Test Checklist Summary

### Unit Tests

| # | Module | Date Tested | Firmware Commit | Pass/Fail |
|---|--------|-------------|-----------------|-----------|
| 1 | IMU — Chip ID | | | ☐ |
| 2 | IMU — Accel data | | | ☐ |
| 3 | IMU — Sleep/wake | | | ☐ |
| 4 | Display — Init and text | | | ☐ |
| 5 | Display — Drawing | | | ☐ |
| 6 | Display — Brightness/on/off | | | ☐ |
| 7 | Battery — Voltage | | | ☐ |
| 8 | Battery — Charging | | | ☐ |
| 9 | Vibration — Pulse | | | ☐ |
| 10 | Vibration — Pattern | | | ☐ |
| 11 | Storage — Save/load | | | ☐ |
| 12 | Storage — Persistence | | | ☐ |
| 13 | Storage — Workouts | | | ☐ |
| 14 | BLE — Advertising | | | ☐ |
| 15 | BLE — Connect/read | | | ☐ |
| 16 | BLE — Time sync | | | ☐ |
| 17 | BLE — Weekday derivation | | | ☐ |
| 18 | Power — Idle timer | | | ☐ |
| 19 | Power — Sleep current | | | ☐ |
| 20 | DateTime — Tick rollover | | | ☐ |
| 21 | DateTime — Leap year | | | ☐ |
| 22 | DateTime — Epoch | | | ☐ |
| 23 | Step counter — Accuracy (regression) | | | ☐ |
| 24 | Step counter — Noise rejection | | | ☐ |
| 25 | Activity — Detection | | | ☐ |
| 26 | Activity — Hysteresis | | | ☐ |
| 27 | Workout — Lifecycle | | | ☐ |
| 28 | UI — Screen cycling | | | ☐ |
| 28b | UI — Timeout/wake | | | ☐ |

### Field Tests

| # | Module | Date Tested | Firmware Commit | Pass/Fail |
|---|--------|-------------|-----------------|-----------|
| 29 | Integration — Battery-dead datetime recovery | | | ☐ |
| 30 | Integration — Walk test (real steps) | | | ☐ |
| 31 | Integration — Workout flow | | | ☐ |
| 32 | Integration — BLE sync | | | ☐ |
| 33 | Integration — Battery life | | | ☐ |
