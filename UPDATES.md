# Project Updates

Update this file in the same change set whenever firmware, hardware, wiring, protocol, tests, scope, or build instructions change. Add newest entries first and include validation performed.

## 2026-07-17 - Phone-Assisted V1 Baseline

### Product and Hardware

- Reframed V1 as a phone-assisted running computer. The phone owns GPS and route capture; the watch owns controls, display, cadence, haptics, BLE telemetry, and local records.
- Standardized the prototype on the Seeed Studio XIAO nRF52840 Sense.
- Replaced the external BMI270 with the Sense board's onboard LSM6DS3TR-C.
- Removed the external LF crystal, TP4056 charger, external regulator, and standalone GPS from the V1 BOM.
- Corrected OLED, button, vibration-driver, battery, and onboard-IMU wiring.
- Updated the expected BOM to $39-64 excluding tools and shipping.
- Replaced speculative multi-day and deep-sleep claims with measurable current and 24-hour runtime gates.

### Firmware

- Added phone-GPS distance, current pace, average pace, accuracy, source, and freshness state to the workout model.
- Added a versioned 10-byte little-endian phone-GPS BLE write packet on characteristic suffix `1006`.
- Replaced raw `DateTime` memory writes with an exact 7-byte little-endian time-sync packet.
- Added range validation and watch-side weekday derivation for time sync.
- Reworked workout telemetry into a packed 20-byte notification that fits the default BLE ATT payload.
- Workout telemetry now reports stopped, active, and paused states once per second while connected.
- Added GPS freshness timeout handling and `PHONE GPS`, `GPS LOST`, and `DISTANCE EST.` display states.
- Added current pace to the workout display.
- Changed controls to short press for wake/navigation, 1-second hold for start/pause/resume, and 3-second hold for stop.
- Added a workout-stop callback that appends completed records to LittleFS.
- Added explicit invalid-time state after boot; unsynchronized workout records use timestamp 0.
- Removed the idle path that powered down the IMU without a wake path and repeatedly wrote flash every second.
- Removed the now-unreachable IMU and MCU sleep APIs so logical idle state is not mistaken for implemented deep sleep.
- Corrected battery measurement to use `PIN_VBAT`, active-low `VBAT_ENABLE`, and P0.17 charge status.
- Fixed LittleFS workout append positioning for the Adafruit filesystem API.

### Sensor and Build System

- Switched PlatformIO to Seeed's supported board platform and exact `seeed-xiao-afruitnrf52-nrf52840-sense` target.
- Pinned Seeed Arduino LSM6DS3 v2.0.7.
- Configured the onboard IMU for 52 Hz, +/-4 g acceleration, and +/-500 dps gyro.
- Removed the incompatible handwritten BMI270 initialization and unused hardware-step interface.
- Removed an unused button-task variable found by compilation.
- Added `firmware/.gitignore` so PlatformIO build output is not tracked.

### Documentation and Tests

- Replaced `DESIGN.md` with the authoritative V1 scope, data ownership, controls, BLE overview, delivery stages, and success criteria.
- Replaced `SYSTEM_ARCHITECTURE.md` with the exact BOM, wiring, BLE byte layouts, storage limitations, power model, and build command.
- Replaced `TESTING.md` with build, wiring, hardware, protocol-negative, storage, cadence, outdoor GPS, and measured-runtime tests.

### Validation

- Installed PlatformIO 6.1.19 in the user Python environment because the CLI was unavailable.
- Confirmed the original `xiaonrf52840` board ID was invalid and replaced it with Seeed's supported target.
- Built successfully after the LSM6DS3 migration.
- Built successfully after the BLE GPS and workout-persistence changes.
- Built successfully after the exact Sense target, battery, time-validity, controls, and UI changes.
- Latest measured build before documentation finalization: 149,212 bytes flash (18.4%) and 15,612 bytes RAM (6.6%).
- Hardware and field tests remain pending until the physical prototype and phone GPS sender are available.
