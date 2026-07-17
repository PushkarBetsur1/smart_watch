# Version 1 System Architecture

## System Boundary

```text
                           BLE, 1 Hz GPS updates
Phone GPS and route app  <------------------------>  XIAO nRF52840 Sense
                                                          |
                             +----------------------------+------------------+
                             |                            |                  |
                      onboard LSM6DS3               SPI OLED          button / motor
```

The phone is part of the V1 system. The watch is usable as a clock and step prototype without it, but accurate workout distance and pace require the phone connection.

## Bill of Materials

| Item | Selected part | Typical cost |
|---|---|---:|
| MCU and IMU | Seeed XIAO nRF52840 Sense | $16-20 |
| Display | 1.3 inch SPI SSD1306, 128 x 64 | $5-8 |
| Battery | Protected 3.7 V LiPo, 400-500 mAh | $6-9 |
| Haptic motor | 3 V coin motor | $1-3 |
| Motor driver | AO3400A or similar 3.3 V logic-level N-MOSFET | $0.25-1 |
| Flyback diode | 1N5819 | $0.10-1 |
| Gate resistor / pulldown | 100 ohm / 100 kohm | $0.10-1 |
| Button | Normally-open tactile switch | $0.10-1 |
| Perfboard, wire, connectors | Prototype supplies | $4-8 |
| Strap and printed enclosure | Prototype fitment | $7-12 |
| **Expected total** | Excluding tools and shipping | **$39-64** |

The target is achievable near $60 if a printer, soldering tools, wire, and a USB-C cable are already available. Buy a protected battery from a reputable seller. Do not connect an unverified LiPo polarity to the XIAO.

### Parts Not Used in V1

- External BMI270 breakout
- External 32.768 kHz crystal
- TP4056 or other second charger
- External voltage regulator
- Standalone GNSS module or antenna
- Barometer
- Optical heart-rate sensor

The Sense board already contains the IMU, LF crystal, USB-C charger, regulator, and battery ADC circuit.

## Exact Wiring

All `D` numbers below are XIAO Arduino pin names, not raw nRF52 GPIO numbers.

### OLED

| OLED signal | XIAO pin | Raw nRF pin | Notes |
|---|---|---|---|
| VCC | 3V3 | - | Do not use 5 V logic |
| GND | GND | - | Common ground |
| SCK / CLK | D8 | P1.13 | Hardware SPI clock |
| MOSI / DIN | D10 | P1.15 | Hardware SPI data |
| CS | D7 | P1.12 | Chip select |
| DC | D2 | P0.28 | Data/command |
| RST / RES | D3 | P0.29 | Display reset |

Buy the SPI display variant. A four-pin I2C SSD1306 module is not a drop-in replacement for this wiring.

### Button

```text
D0 (P0.02) ---- normally-open button ---- GND
```

Firmware enables the internal pull-up. No external pull-up is required.

### Vibration Motor

```text
3V3 ---------------- motor +
                      motor - ----+---- drain, AO3400A
                                  |
                     1N5819 across motor
                     cathode toward 3V3

D6 (P1.11) -- 100 ohm -- gate
gate -------- 100 kohm ----------- GND
source --------------------------- GND
```

Never drive the motor directly from a GPIO. Verify the motor stall current is within the 3V3 rail and transistor limits.

### Battery

Connect the protected LiPo to the XIAO `BAT+` and `BAT-` pads with polarity checked by multimeter before soldering. USB-C charging is handled by the onboard BQ25101-family charger.

The firmware uses the board definitions:

- `PIN_VBAT` -> P0.31 ADC input
- `VBAT_ENABLE` -> P0.14, driven low only while sampling
- Digital pin 23 -> P0.17 active-low charge status

The button on D0 does not share the battery ADC pin.

### Onboard IMU

The LSM6DS3TR-C is already wired to the board's internal `Wire1` bus and powered through the board variant. Do not connect an external sensor to D4/D5 for the V1 IMU path.

Firmware settings:

| Setting | Value |
|---|---:|
| Accelerometer rate | 52 Hz |
| Accelerometer range | +/-4 g |
| Accelerometer bandwidth | 50 Hz |
| Gyroscope rate | 52 Hz |
| Gyroscope range | +/-500 dps |

## Firmware Components

```text
main.cpp
  +-- sensor task: LSM6DS3 -> step counter -> cadence/activity -> workout
  +-- UI task: button events and display refresh
  +-- BLE task: steps, battery, and compact workout notifications
  +-- power task: periodic persistence and idle state tracking

app/
  +-- datetime: RAM clock, invalid until each boot's phone sync
  +-- step_counter: prototype wrist peak detector
  +-- activity: cadence-based idle/walk/run classification
  +-- workout: lifecycle, GPS ownership, pace, calories
  +-- ui: home, workout, and daily stats screens

hal/
  +-- imu: Seeed LSM6DS3 driver wrapper
  +-- ble_service: packed protocol parsing and notifications
  +-- display, battery, storage, vibration, power
```

## BLE Protocol Version 1

Service UUID: `00001000-0000-1000-8000-00805f9b34fb`

All integers are unsigned little-endian.

### Time Sync Write - suffix `1004`

Exactly 7 bytes:

| Offset | Type | Meaning |
|---:|---|---|
| 0 | u16 | Year, 2024-2099 |
| 2 | u8 | Month, 1-12 |
| 3 | u8 | Day, 1-31 |
| 4 | u8 | Hour, 0-23 |
| 5 | u8 | Minute, 0-59 |
| 6 | u8 | Second, 0-59 |

The watch derives weekday. Invalid payload lengths, ranges, and years are ignored.

### Phone GPS Write - suffix `1006`

Exactly 10 bytes:

| Offset | Type | Meaning |
|---:|---|---|
| 0 | u8 | Protocol version, must be `1` |
| 1 | u8 | Flags; bit 0 means valid fix |
| 2 | u32 | Cumulative active-workout distance, cm |
| 6 | u16 | Filtered horizontal speed, cm/s |
| 8 | u16 | Horizontal accuracy, cm |

The phone sends this at 1 Hz while a workout is active. Cumulative distance excludes paused time. A zero speed means pace unavailable, not infinite pace.

Example: version 1, valid fix, 1234.56 m, 3.25 m/s, 4.20 m accuracy:

```text
01 01 40 E2 01 00 45 01 A4 01
```

### Workout Notification - suffix `1003`

Exactly 20 bytes so it fits the default BLE ATT payload:

| Offset | Type | Meaning |
|---:|---|---|
| 0 | u8 | Protocol version, `1` |
| 1 | u8 | State: 0 stopped, 1 active, 2 paused |
| 2 | u8 | Flags; bit 0 means phone GPS update received in last 5 s |
| 3 | u8 | Activity: 0 idle, 1 walking, 2 running |
| 4 | u32 | Workout steps |
| 8 | u32 | Distance, m |
| 12 | u32 | Active elapsed time, s |
| 16 | u16 | Current pace, s/km; 0 means unavailable |
| 18 | u16 | Average pace, s/km; 0 means unavailable |

Daily steps (`1001`) is a 4-byte u32 notification. Battery (`1002`) is a 1-byte percentage notification.

## Storage

LittleFS stores settings, the latest daily total, last synchronized timestamp for diagnostics, and completed workout records. Time is intentionally invalid after reboot because flash cannot measure how long power was absent.

Workout records currently append to `/workouts.bin`. Back up or format storage during development; record rotation is required before long-term deployment.

## Power Model

The current firmware uses FreeRTOS delays and turns the OLED off, but it continues sampling the IMU at 52 Hz for steps. `POWER_IDLE` is a logical state, not System OFF.

Measure these states on assembled hardware:

| State | Initial engineering target |
|---|---:|
| Display on, BLE connected | < 15 mA average |
| Display off, BLE connected | < 5 mA average |
| Display off, BLE advertising | < 3 mA average |
| Motor pulse | Record peak and supply droop |

These are test thresholds, not claimed measurements. At 5 mA average, a healthy 400 mAh cell has a theoretical 80-hour runtime before regulator, cutoff, temperature, and capacity losses. Use measured current and an 80% usable-capacity factor for planning.

## Build

From `firmware/`:

```powershell
py -m platformio run
```

The project uses Seeed's PlatformIO board package and the exact Sense target `seeed-xiao-afruitnrf52-nrf52840-sense`.