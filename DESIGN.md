# Phone-Assisted Running Watch - Version 1 Design

## Product Definition

Version 1 is a wrist-worn running computer that uses a paired phone for GPS. The watch owns workout controls, display, cadence estimate, step estimate, haptics, local records, and BLE. The phone owns location permission, GPS sampling, route recording, and cumulative GPS distance.

This split keeps the first prototype close to $60, avoids a large GNSS antenna and its power load, and lets the running workflow be tested before committing to standalone GPS hardware.

## V1 Goals

- Current time after phone synchronization
- Start, pause, resume, and stop a run from the watch
- Elapsed active time
- Phone-GPS distance
- Current and average pace
- Wrist-derived cadence and step estimate
- Haptic control feedback and future interval alerts
- Battery monitoring
- Local completed-workout records
- Live workout synchronization over BLE
- At least one full day of normal prototype use, to be verified on hardware

## Explicit Non-Goals

- Standalone GPS
- Optical heart rate
- Running power
- Elevation
- VO2 max
- Waterproofing
- Apple-level automatic workout detection
- Medical or training-grade calorie accuracy

Standalone GNSS and optical heart rate are possible later revisions, not hidden assumptions in V1.

## Hardware

### Controller and Motion Sensor

Use the **Seeed Studio XIAO nRF52840 Sense**, not the base XIAO plus an external IMU. The Sense board provides:

- nRF52840 BLE MCU
- Onboard LSM6DS3TR-C six-axis IMU
- Onboard 32.768 kHz LF crystal
- USB-C programming and charging
- Built-in switched battery measurement
- 2 MB external flash, although V1 uses internal LittleFS for its small records

Do not add a BMI270 breakout, external watch crystal, TP4056 charger, or separate regulator to V1.

### Display, Controls, and Haptics

- 1.3 inch, 128 x 64 monochrome SSD1306 OLED using four-wire SPI
- One normally-open tactile button from D0 to GND
- One 3 V coin vibration motor driven through a logic-level N-channel MOSFET
- Flyback diode across the motor
- Display shuts off after five seconds by default

### Battery

- Protected 3.7 V LiPo, 400-500 mAh
- Battery connects directly to the XIAO battery pads with verified polarity
- Charging uses the XIAO onboard charger through USB-C
- V1 must not include a second charger module

Battery-life numbers remain targets until measured on assembled hardware. The firmware keeps the display off when idle but currently samples motion continuously for daily steps. It does not claim a sub-20 uA whole-system sleep state.

## Running Data Model

### GPS Ownership

During an active workout, the phone sends a GPS update once per second containing a valid-fix flag, cumulative workout distance, filtered horizontal speed, and horizontal accuracy.

The phone must reject poor locations and prevent cumulative distance from moving backward. The watch treats phone distance as authoritative after the first valid update.

Before the first valid phone update, the watch labels step-based distance as an estimate. If updates stop for more than five seconds after GPS has been acquired, the watch displays `GPS LOST` and retains the last accepted distance.

### Pace

Current pace is calculated from phone speed:

```text
pace_sec_per_km = 100000 / speed_cm_per_sec
```

Average pace is calculated from active elapsed time and GPS distance:

```text
average_pace_sec_per_km = elapsed_ms / distance_m
```

The phone should filter speed over approximately 5-10 seconds. It must not send noisy raw point-to-point speed as a precise current pace.

### Cadence and Steps

The onboard LSM6DS3TR-C is sampled at 52 Hz. V1 uses an acceleration-magnitude peak detector. This is prototype logic and must be calibrated from recorded wrist data before any accuracy claim is made.

Cadence is independent from GPS. GPS distance remains authoritative during a workout.

## Workout Controls

- Short press: wake display or cycle screens
- Hold for at least 1 second: start, pause, or resume
- Hold for at least 3 seconds: stop and save the workout

Completed workouts are written to LittleFS. A record contains timestamp when available, duration, steps, distance, calories, and detected activity.

The board has no battery-backed real-time clock. After every reboot, time remains explicitly invalid until phone sync. Records use timestamp `0` when time is invalid rather than pretending a saved pre-shutdown clock is current.

## BLE Contract

The custom service UUID is `00001000-0000-1000-8000-00805f9b34fb`.

| Characteristic | UUID suffix | Direction | Size |
|---|---:|---|---:|
| Daily steps | `1001` | Watch notify/read | 4 bytes |
| Battery | `1002` | Watch notify/read | 1 byte |
| Workout telemetry | `1003` | Watch notify/read | 20 bytes |
| Time sync | `1004` | Phone write | 7 bytes |
| Phone GPS | `1006` | Phone write | 10 bytes |

All multibyte values are unsigned little-endian. Protocol version 1 uses open GATT for prototype development. Do not send route coordinates to the watch. Pairing and bonding are required before treating this as a personal-data product.

## Firmware Architecture

```text
Phone location service
        |
        | BLE GPS update, 1 Hz
        v
BLE service --> Workout manager --> UI / local record
                    ^
                    |
LSM6DS3 --> step and cadence estimator
```

The hardware abstraction layer contains display, IMU, battery, storage, vibration, BLE, and power-state helpers. Application modules own time, activity, workout calculations, step estimation, and screens.

## Delivery Stages

### Stage 1 - Bench Prototype

- Assemble the exact BOM in `SYSTEM_ARCHITECTURE.md`
- Flash and run the complete firmware
- Validate display, button, motor, IMU, battery ADC, storage, and BLE
- Send synthetic phone-GPS packets with nRF Connect

### Stage 2 - Wearable and Phone GPS

- Build a minimal phone companion that records GPS in the foreground
- Stream version 1 GPS packets at 1 Hz
- Strap the watch securely to the wrist
- Run measured outdoor routes and compare against a trusted reference

### Stage 3 - Calibration and Power

- Capture labeled walking, running, desk-work, and vehicle IMU logs
- Tune cadence and false-step rejection from those logs
- Measure current with display on, display off, BLE connected, and during vibration
- Set battery expectations from measurements, not component datasheets

### Stage 4 - Optional Standalone Hardware

Only after V1 is useful, evaluate a modern low-power GNSS module, antenna placement, larger battery, and enclosure changes. That is expected to add roughly $23-40.

## Success Criteria

- A run can be started, paused, resumed, stopped, and recovered from storage
- GPS distance is within 3% of a trusted reference on an open-sky 5 km route
- Average pace is within 5 seconds per kilometer of the reference
- Cadence is within 5 steps per minute of a manually counted 60-second interval
- A GPS disconnect is visible within 6 seconds and does not reset distance
- No false claim is made for unsynchronized time, estimated distance, or unmeasured battery life