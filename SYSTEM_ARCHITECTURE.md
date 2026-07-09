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

| Parameter       | Value                        |
|-----------------|------------------------------|
| Part            | Nordic nRF52840              |
| Core            | ARM Cortex-M4F @ 64 MHz     |
| Flash           | 1 MB internal               |
| RAM             | 256 KB                      |
| Radio           | BLE 5.0, 2.4 GHz           |
| GPIO            | 48 pins                     |
| ADC             | 12-bit, 8 channels          |
| Supply Voltage  | 1.7V – 5.5V                |
| Sleep Current   | ~3 µA (System OFF)         |
| LFCLK Source    | External 32.768 kHz crystal |
| Package         | QFN48 (custom PCB)          |

**Prototype dev board:** Seeed XIAO nRF52840 (~$10–16)
- Smallest nRF52840 form factor available
- Built-in USB-C, LiPo charge circuit, and VBAT sense pin (P0.31)
- Pin names used throughout this document refer to XIAO Arduino pin numbers (D0–D10, A0)

---

### IMU — Bosch BMI270

| Parameter         | Value                          | Notes |
|-------------------|--------------------------------|-------|
| Part              | Bosch BMI270                   | |
| Axes              | 6 (3-axis accel + 3-axis gyro) | |
| Interface         | I2C (up to 400 kHz) / SPI      | I2C used in V1 |
| Supply Voltage    | 1.8V (logic) / 3.3V tolerant   | Most breakouts include level shifting |
| Current (active)  | 685 µA                         | Full accel + gyro |
| Current (low-power step mode) | ~25 µA            | Step counter only, accel low-power |
| Current (sleep)   | 3.5 µA                         | |
| Accel Range       | ±2g / ±4g / ±8g / ±16g        | ±4g used in V1 |
| Gyro Range        | ±125 to ±2000 °/s              | |
| Step Counter      | Hardware engine (on-chip)      | V1 primary — MCU sleeps between interrupts |
| I2C Address       | 0x68 (SDO→GND) / 0x69 (SDO→VCC) | |
| Approx. Cost      | $10–15 (breakout board)        | |

**Prototype breakout:** Adafruit BMI270 or SparkFun BMI270
> Check stock before ordering — BMI270 breakouts can be scarce. MPU6050 is a fallback but lacks the onboard step engine.

---

### Display — 1.3" OLED

| Parameter       | Value                    | Notes |
|-----------------|--------------------------|-------|
| Size            | 1.3 inches               | |
| Resolution      | 128 × 64 pixels          | |
| Controller      | SSD1306                  | |
| Interface       | **SPI** (required)       | Do not buy I2C variant — SPI is faster and matches pin assignment |
| Supply Voltage  | 3.3V                     | |
| Active Current  | ~20–30 mA                | Depends on pixel fill |
| Sleep Current   | ~5 µA (display off cmd)  | |
| Color           | White on black           | |
| Approx. Cost    | $8–12                    | |

---

### 32.768 kHz Timekeeping Crystal

| Parameter       | Value                     | Notes |
|-----------------|---------------------------|-------|
| Frequency       | 32.768 kHz                | Standard watch crystal |
| Purpose         | LFCLK source for nRF52840 RTC | |
| Drift           | ~20 ppm (~1.7 s/day)      | vs ~250 ppm (~20 s/day) for internal RC oscillator |
| Current         | ~1 µA                     | |
| Package         | Through-hole or SMD 3.2×1.5mm | |
| Approx. Cost    | ~$1                       | Required — do not skip |

> Without this crystal the watch will drift too fast for real-world use. The nRF52840 requires a 32.768 kHz crystal connected to XL1/XL2 pins to feed the LFCLK source for the RTC.

---

### Power System

#### Battery

| Parameter       | Value                    | Notes |
|-----------------|--------------------------|-------|
| Chemistry       | LiPo / Li-Ion            | |
| Voltage         | 3.7V nominal             | |
| Capacity        | 300–500 mAh              | 400 mAh target |
| Connector       | JST-PH 2-pin             | Match polarity to charge circuit |
| Approx. Cost    | $6–9                     | |

#### Charge Controller

| Parameter       | Value                    | Notes |
|-----------------|--------------------------|-------|
| Part (proto)    | TP4056 module            | ~$1–2, easy to breadboard |
| Part (PCB)      | MCP73831                 | Compact SOT-23-5, configurable charge current |
| Charge Current  | 100–500 mA (configurable via PROG resistor) | |
| Charge Interface| USB-C                    | |
| Protection      | Undervoltage cutoff, overcurrent | Use DW01A or similar if MCP73831 used alone |

> **Prototype note:** A TP4056 module includes protection circuitry and is the fastest path to a working charge circuit. Swap to MCP73831 for the custom PCB.

#### Voltage Regulator

| Parameter       | Value                    | Notes |
|-----------------|--------------------------|-------|
| Part            | AP2112K-3.3              | |
| Output          | 3.3V fixed               | |
| Max Current     | 600 mA                   | |
| Quiescent Current | 55 µA                  | **Verify against sleep budget before PCB commit** — dominates at low load |
| Alternative     | XC6206P332MR (Torex)     | 1 µA Iq — better for sleep power |
| Approx. Cost    | ~$0.50                   | |

> The XIAO nRF52840 has an onboard regulator. For the custom PCB, evaluate a lower-Iq part (e.g., Torex XC6206) — 55 µA from the AP2112K will be ~50% of your deep-sleep budget.

#### Battery Voltage Monitoring

| Parameter       | Value                    | Notes |
|-----------------|--------------------------|-------|
| Method          | ADC via voltage divider   | VBAT/2 → nRF52840 12-bit ADC |
| Switch          | GPIO-controlled low-side N-FET | Powers divider only during measurement |
| ADC Pin (XIAO)  | P0.31 (built-in divider)  | XIAO has built-in VBAT/2 on P0.31 — prototype only, no GPIO switch |
| ADC Pin (PCB)   | AIN4 (P0.28)              | Implement switched divider for custom PCB |
| Divider Switch FET | 2N7002 (N-channel)     | Gate driven by MCU GPIO |

> The XIAO's built-in divider is always-on — acceptable for prototyping but not ideal for sleep current. On the custom PCB, add a GPIO-switched divider.

---

### Button

| Parameter       | Value                      | Notes |
|-----------------|----------------------------|-------|
| Type            | Tactile momentary switch   | |
| Quantity        | 1 (V1), 2 (V2)            | |
| Debounce        | Software (FreeRTOS delay)  | Hardware RC (100Ω + 100nF) optional |
| Pull            | Internal pull-up (nRF52840) | No external resistor needed |
| Approx. Cost    | < $1                       | Buy 5× — they fail |

---

### Vibration Motor

| Parameter       | Value                        | Notes |
|-----------------|------------------------------|-------|
| Type            | Coin/pancake vibration motor | |
| Voltage         | 3.0V                         | |
| Current         | ~60–80 mA                   | Exceeds GPIO drive — must use driver |
| Driver          | 2N7002 N-channel MOSFET      | Gate → MCU GPIO, Drain → motor –, Source → GND |
| Flyback Diode   | 1N4148                       | Required across motor terminals |
| Control         | GPIO (on/off) or PWM (intensity) | |
| Approx. Cost    | $2–4 motor + <$1 driver parts |

---

### Passives & Discretes

| Component | Value / Part | Qty | Purpose | Approx. Cost |
|-----------|-------------|-----|---------|--------------|
| I2C pull-up resistors | 4.7 kΩ | 2 | SDA and SCL pull-ups to 3.3V | <$1 (kit) |
| Battery divider resistors | 100 kΩ + 100 kΩ | 2 | VBAT/2 voltage divider for ADC | <$1 (kit) |
| Divider switch FET | 2N7002 | 1 | GPIO-controlled battery sense enable | <$1 (pack) |
| Motor driver MOSFET | 2N7002 | 1 | Vibration motor drive | <$1 (pack) |
| Motor flyback diode | 1N4148 | 1 | Motor back-EMF protection | <$1 (pack) |
| Button debounce cap | 100 nF | 1 | Optional RC debounce | <$1 (kit) |
| Button debounce resistor | 100 Ω | 1 | Optional RC debounce | <$1 (kit) |
| Crystal load capacitors | 12 pF | 2 | Required for crystal oscillator | <$1 (kit) |
| Decoupling caps | 100 nF | 4 | Power supply decoupling per IC | <$1 (kit) |

> **Buy an assorted resistor/capacitor kit** (~$5–10) rather than individual values — cheaper and covers everything above.

---

### Full BOM Summary

| Component | Part | Approx. Cost | Source |
|-----------|------|-------------|--------|
| MCU dev board | Seeed XIAO nRF52840 | $10–16 | Seeed Studio, Digikey |
| IMU | Adafruit/SparkFun BMI270 breakout | $10–15 | Adafruit, SparkFun |
| Display | 1.3" SSD1306 OLED SPI 128×64 | $8–12 | Amazon, AliExpress |
| Battery | LiPo 3.7V 400 mAh JST-PH | $6–9 | Adafruit, Amazon |
| Charge module | TP4056 USB-C module | $1–2 | Amazon, AliExpress |
| Crystal | 32.768 kHz watch crystal | ~$1 | Digikey, Mouser |
| Vibration motor | Coin motor 3V | $2–4 | Adafruit, Amazon |
| MOSFETs | 2N7002 ×5 | <$1 | Digikey (pack) |
| Flyback diode | 1N4148 ×5 | <$1 | Digikey (pack) |
| Tactile button | 6mm momentary ×5 | <$1 | Amazon (pack) |
| Passives kit | Resistors + capacitors | $5–10 | Amazon |
| Breadboard + jumpers | — | $5–10 | Amazon |
| **Total** | | **~$50–75** | |

> Buy 2× of anything under $5 (motors, buttons, MOSFETs) — cheap parts fail and halting a build over a $2 component is frustrating.
> A multimeter is required for current measurements in TESTING.md — buy one now if you don't have one.

---

## Pin Assignment (Prototype — Seeed XIAO nRF52840)

| Function           | XIAO Pin   | nRF GPIO | Interface | Notes |
|--------------------|------------|----------|-----------|-------|
| OLED SCK           | D8 (SCK)   | P1.13    | SPI CLK   | |
| OLED MOSI          | D10 (MOSI) | P1.15    | SPI MOSI  | |
| OLED CS            | D7         | P1.12    | GPIO      | |
| OLED DC            | D2         | P0.28    | GPIO      | Data/Command select |
| OLED RST           | D3         | P0.29    | GPIO      | |
| IMU SDA            | D4 (SDA)   | P0.26    | I2C       | |
| IMU SCL            | D5 (SCL)   | P0.27    | I2C       | |
| IMU INT1           | D1         | P0.03    | GPIO IRQ  | Step interrupt (active high) |
| Button             | D0         | P0.02    | GPIO      | Active low, internal pull-up |
| Vibration Motor    | D6         | P1.11    | GPIO/PWM  | Via 2N7002 MOSFET driver |
| Battery Sense      | P0.31      | P0.31    | AIN7      | XIAO built-in VBAT/2 divider |
| Bat. Sense Enable  | P0.14      | P0.14    | GPIO      | XIAO charge/sense control (check schematic) |
| USB-C Charge Status| P0.17      | P0.17    | GPIO      | XIAO charge LED pin (LOW = charging) |

---

## Communication Interfaces

### I2C Bus

```text
nRF52840 (Master)
    │
    ├── SDA (D4 / P0.26) ──── 4.7kΩ pull-up to 3.3V
    │
    └── SCL (D5 / P0.27) ──── 4.7kΩ pull-up to 3.3V
            │
            └── BMI270 (0x68)
```

Speed: 400 kHz (Fast Mode)

### SPI Bus

```text
nRF52840 (Master)
    │
    ├── SCK  (D8  / P1.13) ──── OLED CLK
    ├── MOSI (D10 / P1.15) ──── OLED Data
    └── CS   (D7  / P1.12) ──── OLED Chip Select
```

Speed: 8 MHz

### BLE

```text
nRF52840 ─── 2.4 GHz ─── Companion App (Phone)
```

Protocol: BLE 5.0, GATT custom service

---

## Power Budget

### Normal Use (15% screen-on)

| Component             | Active Current | Sleep Current | Duty Cycle | Average |
|-----------------------|---------------|---------------|------------|---------|
| nRF52840 (CPU)        | 4.5 mA        | 3 µA          | 5%         | 228 µA  |
| nRF52840 (BLE)        | 8 mA          | 10 µA         | 1%         | 90 µA   |
| BMI270                | 685 µA        | 3.5 µA        | 100%       | 685 µA  |
| OLED Display          | 25 mA         | 0 µA          | 15%        | 3.75 mA |
| Vibration Motor       | 70 mA         | 0 µA          | 0.1%       | 70 µA   |
| Voltage Regulator     | —             | 55 µA         | 100%       | 55 µA   |
| **Total Average**     |               |               |            | **4.9 mA** |

Estimated battery life (400 mAh): ~82 hours / **~3.4 days**

### Active Workout Use (25% screen-on)

| Component             | Duty Cycle | Average |
|-----------------------|------------|---------|
| OLED Display          | 25%        | 6.25 mA |
| All others (same)     | —          | 1.1 mA  |
| **Total Average**     |            | **~7.4 mA** |

Estimated battery life (400 mAh): ~54 hours / **~2.3 days**

### System Deep Sleep (target)

| Component             | Sleep Current |
|-----------------------|---------------|
| nRF52840 (system off) | 3 µA          |
| BMI270 (sleep)        | 3.5 µA        |
| Voltage Regulator     | 2 µA (low-Iq option) |
| Crystal oscillator    | 1 µA          |
| **Total Deep Sleep**  | **< 10 µA** (target < 20 µA) |

This target is referenced by TESTING.md test #19 (Power — Sleep current).

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
Primary (V1): BMI270 hardware step engine
    BMI270 (internal) → Step interrupt → Wake MCU → Increment count → Sleep

Secondary / Future: Software peak detection
    BMI270 (50 Hz) → Raw Accel Data → Low-Pass Filter → Magnitude Calc
        → Adaptive Peak Detection → Cadence Validation → Step Count
```

V1 uses the BMI270's onboard step counter via INT1 interrupt. The MCU stays in sleep between steps, drawing only the BMI270's 3.5 µA idle current. Software peak detection is retained in the codebase for future cadence/gait analysis work but is not the primary step source.

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
                    └── Payload: year, month, day, hour, minute, second
                        (weekday is derived on-watch from date — not trusted from phone)
```

**Security:** V1 uses an open GATT service (no pairing) — accepted prototype tradeoff. V2 targets LE Secure Connections pairing/bonding to prevent unauthorized reads of workout history or writes to time/settings.

---

## Date/Time Tracking

The watch maintains a full date/time struct in RAM, incremented by the RTC:

```text
struct DateTime {
    uint16_t year;      // 2024-2099
    uint8_t  month;     // 1-12
    uint8_t  day;       // 1-31
    uint8_t  weekday;   // 0=Mon, 6=Sun (always derived from date, never trusted from BLE)
    uint8_t  hour;      // 0-23
    uint8_t  minute;    // 0-59
    uint8_t  second;    // 0-59
};
```

- RTC interrupt fires every second, increments the struct (handles month lengths, leap years)
- BLE time sync writes year/month/day/hour/minute/second; watch re-derives weekday internally using Zeller's congruence to prevent phone sending incorrect weekday
- On shutdown/sleep, current date/time is saved to flash
- On boot, date/time is restored from flash (may be stale if battery died — UI shows "Time unset" indicator until BLE sync)

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
