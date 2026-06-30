# System Architecture

## Hardware Block Diagram

```text
                         ┌─────────────────────────────────────┐
                         │           nRF52840 MCU               │
                         │                                     │
                         │  ┌───────────┐  ┌───────────────┐  │
                         │  │ ARM M4F   │  │  BLE Radio    │  │
                         │  │ 64MHz     │  │  2.4GHz       │  │
                         │  └───────────┘  └───────────────┘  │
                         │                                     │
                         │  ┌───────────┐  ┌───────────────┐  │
                         │  │ 1MB Flash │  │  256KB RAM    │  │
                         │  └───────────┘  └───────────────┘  │
                         │                                     │
                         └──┬──────┬──────┬──────┬──────┬─────┘
                            │      │      │      │      │
                SPI         │      │ I2C  │      │ GPIO │  GPIO
               ┌────────────┘      │      │      │      └────────────┐
               │                   │      │      │                   │
               ▼                   ▼      │      ▼                   ▼
      ┌─────────────────┐  ┌───────────┐ │  ┌────────┐    ┌─────────────┐
      │   1.3" OLED     │  │  BMI270   │ │  │ Button │    │  Vibration  │
      │   128x64        │  │  6-axis   │ │  │        │    │  Motor      │
      │   SSD1306       │  │  IMU      │ │  └────────┘    └─────────────┘
      └─────────────────┘  └───────────┘ │
                                          │
                                          ▼
                                 ┌─────────────────┐
                                 │  Power System   │
                                 │                 │
                                 │  LiPo Battery   │
                                 │  300-500 mAh    │
                                 │                 │
                                 │  USB-C Charge   │
                                 │  IC (MCP73831)  │
                                 └─────────────────┘
```

---

## Component List

### Microcontroller

| Parameter       | Value                     |
|-----------------|---------------------------|
| Part            | Nordic nRF52840           |
| Core            | ARM Cortex-M4F @ 64 MHz  |
| Flash           | 1 MB                     |
| RAM             | 256 KB                   |
| Radio           | BLE 5.0, 2.4 GHz        |
| GPIO            | 48 pins                  |
| ADC             | 12-bit, 8 channels       |
| Supply Voltage  | 1.7V - 5.5V             |
| Sleep Current   | ~3 µA                    |
| Package         | QFN48 (custom PCB)       |

Dev board for prototype: Adafruit Feather nRF52840 Express or Nordic nRF52840-DK

---

### IMU — Bosch BMI270

| Parameter        | Value                        |
|------------------|------------------------------|
| Part             | Bosch BMI270                 |
| Axes             | 6 (3-axis accel + 3-axis gyro) |
| Interface        | I2C (up to 400 kHz) / SPI   |
| Supply Voltage   | 1.8V                        |
| Current (active) | 685 µA                      |
| Current (sleep)  | 3.5 µA                      |
| Accel Range      | ±2g, ±4g, ±8g, ±16g        |
| Gyro Range       | ±125 to ±2000 °/s           |
| Step Counter     | Hardware engine (on-chip)    |
| I2C Address      | 0x68 (default) / 0x69       |

Breakout board for prototype: SparkFun BMI270 or Adafruit BMI270

---

### Display — 1.3" OLED

| Parameter       | Value               |
|-----------------|---------------------|
| Size            | 1.3 inches          |
| Resolution      | 128 x 64 pixels     |
| Controller      | SSD1306             |
| Interface       | SPI (preferred) or I2C |
| Supply Voltage  | 3.3V                |
| Active Current  | ~20-30 mA           |
| Color           | White on black       |

---

### Power System

| Component           | Part / Spec               |
|---------------------|---------------------------|
| Battery             | LiPo, 3.7V, 300-500 mAh |
| Charge Controller   | MCP73831 (or TP4056)     |
| Charge Interface    | USB-C                    |
| Voltage Regulator   | 3.3V LDO (AP2112K)      |
| Battery Monitoring  | nRF52840 ADC via voltage divider |
| Charge Current      | 100-500 mA configurable  |
| Protection          | Undervoltage cutoff, overcurrent |

---

### Button

| Parameter       | Value                    |
|-----------------|--------------------------|
| Type            | Tactile momentary switch |
| Quantity        | 1 (V1), 2 (V2)          |
| Debounce        | Hardware RC + software   |
| Pull            | Internal pull-up (nRF52840) |

---

### Vibration Motor

| Parameter       | Value                    |
|-----------------|--------------------------|
| Type            | Coin/pancake vibration motor |
| Voltage         | 3.0V                    |
| Current         | ~60-80 mA               |
| Driver          | MOSFET (2N7002) + flyback diode |
| Control         | GPIO PWM from MCU       |

---

## Pin Assignment (Prototype — Dev Board)

| Function         | nRF52840 Pin | Interface | Notes               |
|------------------|--------------|-----------|---------------------|
| OLED SCK         | P0.25        | SPI CLK   |                     |
| OLED MOSI        | P0.24        | SPI MOSI  |                     |
| OLED CS          | P0.05        | GPIO      |                     |
| OLED DC          | P0.06        | GPIO      | Data/Command select |
| OLED RST         | P0.07        | GPIO      |                     |
| IMU SDA          | P0.26        | I2C       |                     |
| IMU SCL          | P0.27        | I2C       |                     |
| IMU INT1         | P0.11        | GPIO IRQ  | Step/motion interrupt |
| Button           | P0.13        | GPIO      | Active low, internal pull-up |
| Vibration Motor  | P0.15        | GPIO/PWM  | Via MOSFET driver   |
| Battery Sense    | P0.28        | AIN4      | Voltage divider     |
| USB-C VBUS Detect| P0.12        | GPIO      | Charge status       |

---

## Communication Interfaces

### I2C Bus

```text
nRF52840 (Master)
    │
    ├── SDA (P0.26) ──── 4.7kΩ pull-up to 3.3V
    │
    └── SCL (P0.27) ──── 4.7kΩ pull-up to 3.3V
            │
            └── BMI270 (0x68)
```

Speed: 400 kHz (Fast Mode)

### SPI Bus

```text
nRF52840 (Master)
    │
    ├── SCK  (P0.25) ──── OLED CLK
    ├── MOSI (P0.24) ──── OLED Data
    └── CS   (P0.05) ──── OLED Chip Select
```

Speed: 8 MHz

### BLE

```text
nRF52840 ─── 2.4 GHz ─── Companion App (Phone)
```

Protocol: BLE 5.0, GATT custom service

---

## Power Budget

| Component             | Active Current | Sleep Current | Duty Cycle | Average |
|-----------------------|---------------|---------------|------------|---------|
| nRF52840 (CPU)        | 4.5 mA        | 3 µA          | 5%         | 228 µA  |
| nRF52840 (BLE)        | 8 mA          | 10 µA         | 1%         | 90 µA   |
| BMI270                | 685 µA        | 3.5 µA        | 100%       | 685 µA  |
| OLED Display          | 25 mA         | 0 µA          | 10%        | 2.5 mA  |
| Vibration Motor       | 70 mA         | 0 µA          | 0.1%       | 70 µA   |
| Voltage Regulator     | —             | 55 µA         | 100%       | 55 µA   |
| **Total Average**     |               |               |            | **3.6 mA** |

Estimated battery life (400 mAh): ~111 hours / **~4.6 days**

This is optimistic — real-world usage with more screen-on time will reduce to 2-3 days.

---

## Software Layer Diagram

```text
┌──────────────────────────────────────────────────────┐
│                  Application Layer                     │
│                                                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │ Workout  │ │   UI     │ │ Activity │ │  BLE   │ │
│  │ Manager  │ │ Manager  │ │ Detector │ │ Manager│ │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └───┬────┘ │
│       │             │            │            │      │
└───────┼─────────────┼────────────┼────────────┼──────┘
        │             │            │            │
┌───────┼─────────────┼────────────┼────────────┼──────┐
│       ▼             ▼            ▼            ▼      │
│              Hardware Abstraction Layer               │
│                                                      │
│  ┌───────┐ ┌───────┐ ┌───────┐ ┌─────┐ ┌────────┐ │
│  │  IMU  │ │Display│ │Storage│ │ BLE │ │Battery │ │
│  │Driver │ │Driver │ │Driver │ │Driver│ │ Driver │ │
│  └───┬───┘ └───┬───┘ └───┬───┘ └──┬──┘ └───┬────┘ │
│      │         │         │        │         │      │
└──────┼─────────┼─────────┼────────┼─────────┼──────┘
       │         │         │        │         │
┌──────┼─────────┼─────────┼────────┼─────────┼──────┐
│      ▼         ▼         ▼        ▼         ▼      │
│                    Hardware                          │
│  BMI270     SSD1306    Flash    Radio      ADC     │
└─────────────────────────────────────────────────────┘
```

---

## Data Flow

### Step Counting Pipeline

```text
BMI270 (50 Hz) → Raw Accel Data → Low-Pass Filter → Magnitude Calc
    → Peak Detection → Cadence Validation → Step Count → Storage
```

### Workout Data Flow

```text
Step Counter ─┐
              ├─→ Workout Manager → Storage (Flash)
Activity Det ─┘         │
                        ▼
                  BLE Sync → Phone App
```

### BLE GATT Structure

```text
Custom Fitness Service (UUID: TBD)
│
├── Characteristic: Current Steps    (Read, Notify)
├── Characteristic: Workout Status   (Read, Notify)
├── Characteristic: Battery Level    (Read, Notify)
├── Characteristic: Workout History  (Read)
├── Characteristic: Device Settings  (Read, Write)
└── Characteristic: Date/Time Sync   (Write)
                    └── Payload: year, month, day, weekday, hour, minute, second
```

---

## Date/Time Tracking

The watch maintains a full date/time struct in RAM, incremented by the RTC:

```text
struct DateTime {
    uint16_t year;      // 2024-2099
    uint8_t  month;     // 1-12
    uint8_t  day;       // 1-31
    uint8_t  weekday;   // 0=Mon, 6=Sun
    uint8_t  hour;      // 0-23
    uint8_t  minute;    // 0-59
    uint8_t  second;    // 0-59
};
```

- RTC interrupt fires every second, increments the struct (handles month lengths, leap years)
- BLE time sync writes the full struct from the phone
- On shutdown/sleep, current date/time is saved to flash
- On boot, date/time is restored from flash (may be stale if battery died)

---

## Interrupt Architecture

```text
IMU INT1 (P0.11) ──→ Motion/Step interrupt ──→ Wake from sleep
                                              Update step count

Button (P0.13) ───→ GPIO interrupt ──→ Wake from sleep
                                      UI state change

BLE Event ────────→ SoftDevice IRQ ──→ Connection/data events

Timer ────────────→ RTC interrupt ──→ Periodic sensor polling
                                     Display refresh
                                     Battery check
```

---

## Physical Dimensions (Target)

| Parameter       | Target        |
|-----------------|---------------|
| Watch face      | 38 x 38 mm   |
| Thickness       | 12-14 mm     |
| Strap width     | 20 mm        |
| Weight (w/o strap) | < 40g     |
| Enclosure       | 3D printed (V1), injection molded (V2) |
