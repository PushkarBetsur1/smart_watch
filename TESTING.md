# Version 1 Testing Guide

## Test Policy

A successful compile is necessary but does not prove sensor accuracy, electrical safety, GPS quality, or battery life. Record hardware revision, firmware commit, phone model, phone OS, route, weather, and result for every field test.

Do not advertise an accuracy or runtime number until the matching field test has passed at least three times.

## Required Equipment

- Assembled V1 hardware from `SYSTEM_ARCHITECTURE.md`
- Data-capable USB-C cable
- Multimeter
- Phone with nRF Connect for packet-level tests
- Phone location app or future companion app for running tests
- Trusted reference watch or measured route
- Stopwatch or video for cadence checks
- Optional USB power meter; a proper low-current analyzer is preferred for power work

## 1. Build Verification

From `firmware/`:

```powershell
py -m platformio run
```

Pass criteria:

- Build result is `SUCCESS`
- Target is `seeed_xiao_nrf52840_sense`
- No compiler warnings from project source
- Flash and RAM remain below 80%

To upload after connecting the board:

```powershell
py -m platformio run --target upload
```

If upload cannot find the board, double-press reset to enter the UF2 bootloader and retry.

## 2. Wiring Inspection

Perform before connecting the battery:

- OLED VCC goes to 3V3, not 5 V
- OLED SCK=D8, MOSI=D10, CS=D7, DC=D2, RST=D3
- Button connects D0 to GND only
- Motor is driven by MOSFET, never directly by D6
- Flyback diode polarity matches the wiring diagram
- Grounds are common
- Battery voltage and connector polarity are verified by multimeter
- No TP4056 or second charger is connected

Pass criteria: no continuity short between 3V3 and GND, and battery polarity matches the board markings.

## 3. Boot and Display

1. Power over USB with battery disconnected.
2. Confirm the splash screen appears.
3. Confirm the home screen shows `TIME NOT SET` after every reboot.
4. Wait at least five seconds.
5. Confirm the OLED turns off.
6. Press the button once.

Pass criteria:

- No `IMU ERROR` screen
- Display contains no corruption or overlap
- Button wakes the screen without starting a workout
- Unsynchronized time is never presented as current time

## 4. Onboard IMU

For bench diagnostics, temporarily print `ImuData` from the sensor task at no more than 10 Hz.

Expected values:

- Flat and still: magnitude near 1 g
- Rotating the board transfers gravity among X, Y, and Z
- Shaking produces magnitude peaks above the resting value
- Gyroscope values are near zero while still and change during rotation

Pass criteria:

- Initialization succeeds on the XIAO nRF52840 Sense with no external I2C wiring
- Resting magnitude is between 0.90 g and 1.10 g
- Values update continuously and are finite

The IMU driver uses the board's internal `Wire1` bus. D4 and D5 are unrelated to this test.

## 5. Button and Workout Lifecycle

1. Navigate to the workout screen with short presses.
2. Hold for 1-2 seconds: workout starts.
3. Hold for 1-2 seconds: workout pauses.
4. Wait 10 seconds: elapsed active time must not advance.
5. Hold for 1-2 seconds: workout resumes.
6. Hold for at least 3 seconds: workout stops.
7. Power-cycle and inspect workout storage using a temporary serial diagnostic.

Pass criteria:

- Sequence is stopped -> active -> paused -> active -> stopped
- Pause excludes paused time from elapsed time and average pace
- Each accepted action gives haptic feedback
- A completed record exists after stop
- A 1-2 second hold never stops an active workout
- A sub-one-second press does not trigger a hold action

## 6. Battery ADC and Charging

1. Measure battery voltage directly with a multimeter.
2. Read `battery_read_mv()` over serial.
3. Repeat near full charge and around 3.7 V.
4. Connect USB and read `battery_is_charging()`.
5. Disconnect USB and repeat.

Pass criteria:

- Firmware voltage is within 100 mV of the multimeter after calibration
- Charging status follows the active-low P0.17 charger output
- Repeated ADC reads do not interfere with D0 button input
- No pin or component becomes warm

The percentage curve is currently linear from 3.3 V to 4.2 V and is only a rough estimate. Monotonic behavior matters more than nominal percentage accuracy in V1.

## 7. BLE Advertising

1. Scan with nRF Connect.
2. Find device name `SmartWatch`.
3. Connect and locate service `00001000-0000-1000-8000-00805f9b34fb`.
4. Subscribe to `1001`, `1002`, and `1003` notifications.
5. Disconnect and reconnect.

Pass criteria:

- Device appears within five seconds
- Connection succeeds without pairing for prototype V1
- Steps and battery notifications arrive
- Advertising resumes after disconnect

## 8. Time Sync Protocol

Write exactly seven bytes to characteristic suffix `1004`:

```text
[year_lo, year_hi, month, day, hour, minute, second]
```

Example for 2026-07-17 14:30:00:

```text
EA 07 07 11 0E 1E 00
```

Pass criteria:

- Home screen updates within one second
- Correct weekday is derived by the watch
- Six-byte and eight-byte writes are ignored
- Month 0, February 31, hour 24, and year outside 2024-2099 are ignored
- Reboot returns to `TIME NOT SET`

## 9. Phone GPS Protocol

Start a workout, then write exactly ten bytes to characteristic suffix `1006`:

```text
[version, flags, distance_cm_u32_le, speed_cm_s_u16_le, accuracy_cm_u16_le]
```

Example: valid fix, 1234.56 m distance, 3.25 m/s speed, 4.20 m accuracy:

```text
01 01 40 E2 01 00 45 01 A4 01
```

Expected display:

- `PHONE GPS`
- Distance approximately `1.23 km`
- Current pace approximately `5:07 /km`

Then stop writing for six seconds.

Pass criteria:

- GPS distance replaces estimated distance
- Current pace calculation is within one second per kilometer of the packet value
- Display changes to `GPS LOST` after five seconds
- Last distance remains visible
- Wrong version, wrong length, and invalid-fix packets do not overwrite workout distance

## 10. Workout Notification Protocol

Subscribe to characteristic suffix `1003`. Decode its 20-byte payload as documented in `SYSTEM_ARCHITECTURE.md`.

Pass criteria:

- Byte 0 is version 1
- State follows active and paused transitions
- GPS-fresh flag clears within six seconds of the last update
- Steps, distance, elapsed time, and pace use little-endian encoding
- Notification length is exactly 20 bytes
- Notifications arrive approximately once per second while connected, including stopped state

## 11. Storage Endurance Behavior

1. Format storage once.
2. Complete three short workouts with different distances.
3. Power-cycle after each workout.
4. Load records in order with `storage_load_workouts()`.

Pass criteria:

- All three records are present once
- Duration and distance match each completed workout
- A workout stopped before phone time sync has timestamp 0
- Formatting removes all records

Current V1 storage appends records without rotation. Do not run an unbounded automated workout-creation test against flash.

## 12. Step and Cadence Calibration

Synthetic sine waves are useful only for regression. Real wrist tests are mandatory.

Run each case for at least five minutes and record raw IMU data:

| Case | Expected behavior |
|---|---|
| Watch still on desk | No steps |
| Typing at desk | Minimal false steps |
| Walking normally | Stable walking cadence |
| Easy run | Stable running cadence |
| Fast run | No cadence clipping below actual value |
| Riding in car | Minimal false steps |

For cadence accuracy, manually count steps for a continuous 60-second running interval and compare with the watch.

Pass criteria for prototype release:

- Running cadence within 5 steps/minute in three 60-second trials
- Walking and running total steps within 10% over separate 1,000-step trials
- Desk and vehicle tests each add fewer than 20 false steps in 30 minutes

These thresholds are not currently proven.

## 13. Outdoor GPS Field Test

Use an open-sky route of at least 5 km and a trusted reference device or surveyed route.

1. Start phone location capture and the watch workout together.
2. Run the route without stopping.
3. Record reference and watch distance at each kilometer.
4. Repeat on three different days.
5. Repeat once near trees or moderate buildings to expose degraded reception.

Pass criteria:

- Open-sky final distance within 3% of reference
- Average pace within 5 seconds/km of reference
- No backward distance jumps
- No single accepted GPS update adds an implausible jump
- Watch reports `GPS LOST` during a forced phone disconnect

The phone app, not watch firmware, is responsible for rejecting poor GPS points and filtering speed.

## 14. Power and Runtime

Measure assembled hardware current in these conditions:

1. Display on, BLE connected, active workout
2. Display off, BLE connected, active workout
3. Display off, advertising, no workout
4. Motor active

Initial engineering thresholds:

- Display on and connected: under 15 mA average
- Display off and connected: under 5 mA average
- Display off and advertising: under 3 mA average
- No reset or OLED corruption during motor start

Then run a fully charged battery to automatic cutoff under a written usage script. Pass criterion for V1 is at least 24 hours. Increase the target only after measured results support it.

## Release Checklist

| Check | Result |
|---|---|
| Firmware builds for exact Sense target | [ ] |
| Wiring inspection passed | [ ] |
| Boot/display/time-invalid behavior passed | [ ] |
| IMU bench test passed | [ ] |
| Full workout lifecycle and persistence passed | [ ] |
| Battery voltage and charging passed | [ ] |
| BLE reconnect passed | [ ] |
| Time packet negative tests passed | [ ] |
| GPS packet and timeout tests passed | [ ] |
| 1,000-step walk and run tests passed | [ ] |
| Three outdoor 5 km tests passed | [ ] |
| Measured 24-hour runtime passed | [ ] |