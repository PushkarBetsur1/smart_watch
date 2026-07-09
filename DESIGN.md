# DIY Fitness Smartwatch — Version 1 Design

## Overview

This project is a custom-built fitness smartwatch focused on core activity tracking while maintaining low hardware cost, low power consumption, and a modular firmware architecture. The goal is to create a wearable capable of tracking workouts, counting steps, estimating distance, displaying the current time, and synchronizing data with a companion application over Bluetooth Low Energy (BLE).

The project will begin as a development-board prototype before evolving into a custom PCB and wearable enclosure.

---

# Project Goals

## Primary Objectives

* Display current time
* Accurate step counting
* Walking/running detection
* Workout timer
* Distance estimation
* Pace estimation
* Calorie estimation
* Battery monitoring
* Bluetooth synchronization
* Store workout history
* Low-power operation

## Future Objectives

* Heart rate monitoring
* GPS tracking
* Sleep tracking
* OTA firmware updates
* Waterproof enclosure
* Custom PCB Revision 2
* Companion mobile application

---

# Hardware

## Microcontroller

**Selected MCU**

* Nordic nRF52840

Reasons:

* Excellent BLE support
* Extremely low power consumption
* Large developer ecosystem
* Suitable for wearable devices
* Compatible with Zephyr RTOS and PlatformIO

**Dev Board (Prototype)**

* Seeed XIAO nRF52840

---

## Timekeeping Crystal

* 32.768 kHz watch crystal (external LFCLK source)

Required for accurate RTC operation. The nRF52840's internal RC oscillator drifts ~250 ppm (~20 seconds/day), making daily BLE resyncs necessary. An external 32.768 kHz crystal reduces drift to ~20 ppm (~1.7 seconds/day), acceptable for normal wearable use without frequent syncing.

---

## Sensors

### Required

6-axis IMU

Recommended:

* Bosch BMI270

The BMI270 offers:

* Integrated step counter
* Activity recognition
* Lower power consumption
* Better noise characteristics than older IMUs

---

### Optional (Future)

* MAX30102 Heart Rate Sensor
* GPS Module
* Barometer

---

## Display

* 1.3" OLED
* 128×64 pixels

Power-saving requirements:

* Automatic screen timeout
* Adjustable brightness
* Wake on wrist movement
* Minimal refresh rate while idle

---

## Storage

Persistent storage is required for:

* User settings
* Workout history
* Daily step totals
* Calibration data

Version 1 will use the nRF52840's internal flash.

External SPI flash may be added in later revisions if additional storage becomes necessary.

---

## Inputs

* One multifunction button

Future revisions:

* Two buttons
* Capacitive touch

---

## Haptics

* Coin vibration motor

Used for:

* Workout start/stop confirmation
* Goal notifications
* Low battery alerts
* Button feedback

---

## Power

* 300–500 mAh LiPo battery
* USB-C charging
* Battery voltage monitoring via GPIO-switched voltage divider (divider powered only during ADC measurement to avoid continuous current drain at sleep scale)
* LDO regulator: verify quiescent current (Iq) is compatible with sleep budget before PCB commit — AP2112K is acceptable for prototyping but lower-Iq alternatives may be needed for full battery life targets

Target battery life:

* 2–3 days under normal usage (assumes 15–25% screen-on time during active use; idle-heavy use may reach 4+ days)

---

# Software Architecture

```text
Application
│
├── Workout Manager
├── UI Manager
├── Activity Detection
├── Step Counter
└── BLE Manager
        │
Hardware Abstraction Layer (HAL)
│
├── IMU Driver
├── Display Driver
├── Storage Driver
├── Battery Driver
├── BLE Driver
└── Vibration Driver
```

The HAL separates hardware-specific drivers from application logic, making future hardware revisions significantly easier.

---

# System State Machine

```text
Boot
 │
 ▼
Idle Watch
 │
 ├──────────────┐
 ▼              │
Workout         │
 │              │
 ▼              │
Pause           │
 │              │
 └──────┐       │
        ▼       │
Sync with Phone │
        │       │
        ▼       │
Sleep ◄─────────┘
```

---

# Firmware Modules

```text
src/

app/
    workout.cpp
    workout.h

    step_counter.cpp
    step_counter.h

    activity.cpp
    activity.h

    ui.cpp
    ui.h

hal/
    imu.cpp
    imu.h

    display.cpp
    display.h

    battery.cpp
    battery.h

    ble.cpp
    ble.h

    storage.cpp
    storage.h

    vibration.cpp
    vibration.h

    power.cpp
    power.h

main.cpp
```

---

# Core Features

## Date and Time

Version 1:

* Full date/time tracked (year, month, day, weekday, hour, minute, second)
* Internal MCU RTC increments every second
* Date and time synchronized from phone via BLE
* Stored to flash between sessions
* Handles month lengths and leap years

Future:

* Dedicated external RTC (DS3231) if long-term standalone accuracy becomes necessary

---

## Step Counter

V1 uses the BMI270's onboard hardware step-counting engine. The sensor runs the algorithm internally and raises an interrupt on each detected step, keeping the MCU fully asleep between events and drawing only ~3.5 µA.

Software peak-detection on raw 50–100 Hz data is deferred to a future revision for applications requiring finer-grained cadence analysis or custom gait metrics.

---

## Activity Detection

Activities:

* Idle
* Walking
* Running

Version 1 will use adaptive thresholds and cadence analysis.

Future revisions may incorporate frequency-domain analysis or sensor fusion.

---

## Workout Mode

Tracks:

* Start time
* Elapsed time
* Current steps
* Distance
* Average pace
* Calories
* Battery percentage

---

## Distance Estimation

Formula:

Distance = Step Count × Stride Length

Stride length will be user configurable.

Distance is stride-based, not GPS-derived. The UI labels this readout as an estimate (e.g., "~1.92 km (est.)") so users are not misled into expecting GPS-grade accuracy.

---

## Calories

Estimated from:

* User weight
* Distance
* Activity type

Values are approximate.

---

## Bluetooth Low Energy

Custom GATT service will expose:

* Current workout
* Daily steps
* Battery percentage
* Device settings
* Workout history

Security:

* V1 uses an open GATT service — accepted tradeoff for prototype simplicity
* V2 targets LE Secure Connections pairing/bonding so nearby devices cannot read workout history or write bogus time/settings

Future revisions may add:

* Phone notifications
* OTA firmware updates

---

# Display

## Home Screen

```text
Mon Jun 30

10:42

Steps: 5,382

Battery: 82%
```

## Workout Screen

```text
Running

Time
00:21:43

Steps
2,486

Distance
1.92 km
```

---

# Logging

A lightweight logging interface will be used during development.

Example:

```cpp
LOG_INFO("Workout started");
LOG_WARN("Battery low");
LOG_ERROR("BLE disconnected");
```

Logging can be disabled for production firmware.

---

# Development Stages

## Stage 1 — Prototype

Hardware:

* Seeed XIAO nRF52840 dev board
* BMI270 breakout (Adafruit or SparkFun)
* 1.3" SSD1306 OLED (SPI)
* LiPo battery (300–500 mAh, JST-PH)
* 32.768 kHz watch crystal

Goals:

* Read IMU
* Display time
* Count steps
* Display battery level
* BLE communication

---

## Stage 2 — Firmware Improvements

Goals:

* Adaptive step detection
* Walking/running detection
* Calibration mode
* Persistent storage
* Improved power management

---

## Stage 3 — Custom PCB

Integrate:

* nRF52840
* 32.768 kHz crystal (LFCLK)
* BMI270
* Battery charging
* Display connector
* Vibration motor
* Programming header
* Battery connector
* GPIO-switched battery voltage divider

---

## Stage 4 — Wearable Prototype

Goals:

* Compact enclosure
* Comfortable wrist strap
* USB-C charging
* Optimized battery life

---

# Testing & Calibration

Calibration mode will allow:

* Walking a known number of steps
* Measuring detection accuracy
* Adjusting stride length
* Validating distance estimation

Target accuracy:

* ≥95% step-count accuracy during normal walking and running

---

# Future Features

* Heart rate monitoring
* GPS route tracking
* Sleep tracking
* Interval workouts
* Lap timer
* BLE notifications
* OTA firmware updates
* Data export (CSV/GPX)
* Multiple watch faces
* Weather synchronization

---

# Build System

Prototype:

* PlatformIO

Future evaluation:

* Zephyr RTOS

The project structure is intentionally modular so that migrating to an RTOS later requires minimal changes to application logic.

---

# Success Criteria

* ≥95% step-count accuracy
* 2–3 day battery life
* Stable BLE synchronization
* Persistent workout history
* Responsive user interface
* Modular firmware architecture
* Compact wearable prototype
