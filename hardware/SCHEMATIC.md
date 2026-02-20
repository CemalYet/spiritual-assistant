# Spiritual Assistant - Hardware Design

> **Battery-Powered Smart Adhan Desk Clock with Integrated Touch Display**  
> WT32S3-28S PLUS Module | ESP32-S3 | 4×AA Batteries | 2.8" IPS Capacitive Touch | ~4 months battery life

---

## 📋 Table of Contents

1. [Overview](#overview)
2. [Bill of Materials](#bill-of-materials)
3. [System Architecture](#system-architecture)
4. [WT32S3-28S PLUS Module](#wt32s3-28s-plus-module)
5. [Power Management](#power-management-4aa)
6. [Audio System](#audio-system-max98357a)
7. [Physical Button](#physical-button-wakemutesettings)
8. [Power Switch](#power-switch-onoff)
9. [RTC Module](#rtc-module-ds3231sn)
10. [Carrier Board Pin Mapping](#carrier-board-pin-mapping)
11. [PCB Layout Guidelines](#pcb-layout-guidelines-jlcpcb)
12. [Manufacturing Files](#manufacturing-files)

---

## Overview

| Specification | Value |
|---------------|-------|
| **Main Module** | WT32S3-28S PLUS (SC05 Plus) |
| **MCU** | ESP32-S3R8 (8MB PSRAM, 16MB Flash) |
| **Display** | 2.8" IPS 320×240 Capacitive Touch |
| **Power** | 4×AA Batteries (6V) via MP2359 Buck |
| **Programming** | Via module's USB-C (native USB) |
| **Battery Life** | ~4 months (optimized usage) |
| **Audio** | MAX98357A I2S + 3W Speaker |
| **RTC** | DS3231SN + CR1220 backup |
| **Estimated Cost** | ~€16.36 |

---

## Bill of Materials

| Part | LCSC Part # | Description | Type | Price |
|------|-------------|-------------|------|-------|
| WT32S3-28S PLUS | - | ESP32-S3 + 2.8" IPS Touch Module | Module | €12.00 |
| MAX98357AETE+T | C910544 | I2S 3W Audio Amplifier | Extended | €0.80 |
| DS3231SN | C9866 | Precision RTC ±2ppm | Extended | €2.50 |
| CR1220 Holder | C5365932 | KH-BS1220-2-SMT RTC Backup Battery | Extended | €0.08 |
| MP2359DJ-LF-Z-TP | C52205265 | Buck Converter IC (SOT-23-6) | Extended | €0.16 |
| 10µH 2.2A Inductor | XRTC322512S100MBCA⚠️ | Power inductor for buck (molded, 210mΩ, 1210) | Extended | €0.10 |
| 22µF Cap ×2 | C45783 | Input/output caps for buck | Basic | €0.05 |
| 10µF Cap ×1 | C15525 | MAX98357A decoupling | Basic | €0.02 |
| 100nF Cap ×2 | C1525 | Decoupling caps | Basic | €0.01 |
| SS14 Schottky ×3 | C2480 | 1A 40V diode (power isolation + buck catch) | Basic | €0.15 |
| SPDT Slide Switch | C50377150 | SS12D00G6 ON/OFF power switch | Extended | €0.02 |
| Tactile Switch ×1 | C2939240 | TS-1187F-015E HOOYA SMD push button (wake/mute) | Extended | €0.03 |
| 10K Resistor ×1 | C25744 | Pull-up for button | Basic | €0.01 |
| 4.7K Resistor ×2 | C25900 | I2C pull-ups (SDA/SCL) | Basic | €0.01 |
| 49.9K Resistor 1% ×1 | C25905⚠️ | MP2359 feedback (high side) | Extended | €0.01 |
| 16.2K Resistor 1% ×1 | C25764⚠️ | MP2359 feedback (low side) | Extended | €0.01 |
| Speaker 3W 4Ω | - | 28mm Mini Speaker | - | €0.60 |
| 4×AA Holder | - | 4×AA Battery Holder (buy separately) | - | €0.40 |
| PCB 2-layer | - | 65×50mm Carrier Board | - | €2.00 |
| **TOTAL** | | | | **~€16.41** |

### JLCPCB Part Classification

| Category | Parts | Notes |
|----------|-------|-------|
| **Basic** | MP2359, SS14, Tactile Switch, Passives | No extra fee |
| **Extended** | MAX98357A, DS3231SN, Slide Switch (SS-12D00G4-4MM) | +€3 per unique part |

---

## System Architecture

```
┌───────────────────────────────────────────────────────────────────────────┐
│                        SYSTEM BLOCK DIAGRAM                               │
│                    (Simplified Single Power Path)                         │
└───────────────────────────────────────────────────────────────────────────┘

                        4×AA BATTERY HOLDER
                    ┌─────────────────────────┐
                    │   ═╤═  ═╤═  ═╤═  ═╤═   │
                    │   AA   AA   AA   AA    │  ← 4×1.5V = 6V (fresh)
                    │   ═╧═  ═╧═  ═╧═  ═╧═   │    4×1.0V = 4V (depleted)
                    └─────┬───────────┬──────┘
                          │           │
                      BATT+ (Red)  BATT- (Black)
                          │           │
                       ┌──┴──┐        │
                       │ ON  │  SPDT Slide Switch
                       │ OFF │  (SS-12D00G4-4MM)
                       └──┬──┘        │
                          │           │
                          ▼           ▼
                    ┌─────────────────────────┐
                    │       CARRIER PCB       │
                    │  ┌──────────────────┐   │
                    │  │     MP2359       │   │
                    │  │   Buck Conv.     │   │
                    │  │   6V → 3.3V      │   │
                    │  │    η = 90%       │   │
                    │  └────────┬─────────┘   │
                    │           │             │
                    │           ▼             │
                    │    ┌─────────────┐      │
                    │    │  3.3V RAIL  │      │
                    │    └──────┬──────┘      │
                    │           │             │
                    │  ┌────────┼────────┐    │
                    │  │        │        │    │
                    │  ▼        ▼        ▼    │
                    │ RTC    Module   Audio   │
                    │                         │
                    │  ┌──────────────────┐   │
                    │  │  Physical Button │   │
                    │  │  GPIO8 (Wake/    │   │
                    │  │   Mute/Settings) │   │
                    │  └──────────────────┘   │
                    └─────────────────────────┘
                              │
         ┌────────────────────┼────────────────────┐
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────────────────────────────┐   ┌──────────────────┐
│     WT32S3-28S PLUS MODULE          │   │    DS3231SN      │
│  ┌───────────────────────────────┐  │   │    RTC + Backup  │
│  │  ESP32-S3R8 (8MB PSRAM)       │  │   └────────┬─────────┘
│  │  16MB Flash                   │  │            │
│  │  2.4GHz WiFi + BLE 5.0        │  │   ┌────────┴─────────┐
│  └───────────────────────────────┘  │   │   MAX98357A      │
│  ┌───────────────────────────────┐  │   │   I2S Amplifier  │
│  │  2.8" IPS 320×240 Display     │  │   └────────┬─────────┘
│  │  ST7789V Driver               │  │            │
│  │  Capacitive Touch (GT911)     │  │            ▼
│  └───────────────────────────────┘  │   ┌─────────────────┐
│                                     │   │  3W 4Ω Speaker  │
│  ┌───────────────────────────────┐  │   └─────────────────┘
│  │  USB-C (Power + Programming)  │  │
│  │  Used for: Firmware upload    │  │
│  │           Debug/Serial        │  │
│  │           USB power source    │  │
│  └───────────────────────────────┘  │
│                                     │
│  Expansion Header (2×10 pins)       │
└─────────────────────────────────────┘

              ╔════════════════════════════════════╗
              ║  USB-C works as power source     ║
              ║  AND for programming/debug.       ║
              ║  2nd SS14 feeds RTC + Audio       ║
              ║  from module 3V3 when on USB.     ║
              ╚════════════════════════════════════╝
```

---

## WT32S3-28S PLUS Module

### Module Specifications

| Feature | Value |
|---------|-------|
| **MCU** | ESP32-S3R8N16 |
| **Flash** | 16MB (Quad SPI) |
| **PSRAM** | 8MB (Octal SPI) |
| **Display** | 2.8" IPS, 320×240, ST7789V |
| **Touch** | Capacitive, GT911 Controller |
| **WiFi** | 2.4GHz 802.11 b/g/n |
| **Bluetooth** | BLE 5.0 |
| **USB** | Native USB-OTG |
| **Operating Voltage** | 3.3V |
| **Dimensions** | 77 × 47 × 9 mm |

### Expansion Header Pinout (20-pin)

```
                    WT32S3-28S PLUS
                  Expansion Header
            ┌─────────────────────────┐
            │    ○ ○ ○ ○ ○ ○ ○ ○ ○ ○  │  ← Row 1
            │    ○ ○ ○ ○ ○ ○ ○ ○ ○ ○  │  ← Row 2
            └─────────────────────────┘
                   BOTTOM VIEW

Pin Map (Row 1 - Top):
┌─────┬────────┬────────────────────────────────────┐
│ Pin │ Name   │ Function                           │
├─────┼────────┼────────────────────────────────────┤
│ 1   │ 3V3    │ 3.3V Power Output                  │
│ 2   │ GPIO7  │ I2C SCL → DS3231 SCL               │
│ 3   │ GPIO15 │ I2C SDA → DS3231 SDA               │
│ 4   │ GPIO16 │ Available (RTC INT optional)       │
│ 5   │ GPIO17 │ Available                          │
│ 6   │ GPIO18 │ Available                          │
│ 7   │ GPIO8  │ Available                          │
│ 8   │ GPIO3  │ I2S BCLK → MAX98357 BCLK           │
│ 9   │ GPIO46 │ Available                          │
│ 10  │ GND    │ Ground                             │
└─────┴────────┴────────────────────────────────────┘

Pin Map (Row 2 - Bottom):
┌─────┬────────┬────────────────────────────────────┐
│ Pin │ Name   │ Function                           │
├─────┼────────┼────────────────────────────────────┤
│ 11  │ 5V     │ 5V Input (from USB/Battery system) │
│ 12  │ GPIO6  │ Available                          │
│ 13  │ GPIO5  │ Available                          │
│ 14  │ GPIO4  │ I2S DIN → MAX98357 DIN             │
│ 15  │ GPIO2  │ I2S LRCK → MAX98357 LRC            │
│ 16  │ GPIO1  │ AMP Enable → MAX98357 SD           │
│ 17  │ GPIO42 │ Available                          │
│ 18  │ GPIO41 │ Available                          │
│ 19  │ GPIO40 │ Available                          │
│ 20  │ GND    │ Ground                             │
└─────┴────────┴────────────────────────────────────┘
```

### Internal Display Pins (Pre-wired, NOT on header)

| Function | GPIO | Notes |
|----------|------|-------|
| TFT_CS | GPIO10 | Display Chip Select |
| TFT_DC | GPIO11 | Data/Command |
| TFT_RST | GPIO12 | Reset |
| TFT_BL | GPIO45 | Backlight (PWM) |
| TFT_CLK | GPIO14 | SPI Clock |
| TFT_MOSI | GPIO13 | SPI Data |
| TOUCH_SDA | GPIO38 | I2C Touch Data |
| TOUCH_SCL | GPIO39 | I2C Touch Clock |
| TOUCH_INT | GPIO21 | Touch Interrupt |
| TOUCH_RST | GPIO47 | Touch Reset |

---

## Power Management (4×AA)

### Power Architecture with USB Isolation

The system runs from 4×AA batteries or USB-C. Two SS14 Schottky diodes provide bidirectional isolation: D1 isolates battery power from the module, D2 allows USB power (via module 3V3) to feed RTC and Audio when batteries are off.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    POWER PATH WITH USB ISOLATION                            │
└─────────────────────────────────────────────────────────────────────────────┘

    4×AA BATTERIES (6V)                    MODULE USB-C (5V)
           │                                     │
        ┌──┴──┐                                  │
        │ ON  │  SPDT Slide Switch               │
        │ OFF │  (SS-12D00G4-4MM)                │
        └──┬──┘                                  │
           ▼                                     ▼
    ┌─────────────┐                      ┌──────────────────┐
    │   MP2359    │                      │ Module Internal  │
    │  Buck Conv  │                      │ LDO (AMS1117?)   │
    │  6V → 3.3V  │                      │  5V → 3.3V       │
    └──────┬──────┘                      └────────┬─────────┘
           │                                      │
           │ 3.3V                                 │ 3.3V
           │                                      │
           │  SS14 (Schottky)                     │
           ├───▶├──────────┐                      │
           │               │       ⚡ Diode       │
           │          3.0V │       blocks        │
           │   (after drop)│       reverse!      │
           │               │                      │
           │               │                      │
           │          ┌────┴──────────────────────┤
           │          │                           │
           │          │    WT32S3-28S MODULE      │
           │          │    ┌─────────────────┐    │
           │          └───▶│ 3V3 Pin (I/O)   │◀───┘
           │               │                 │
           │               │ ESP32-S3 Core   │
           │               │ Display Driver  │
           │               │ Touch IC        │
           │               └─────────────────┘
           │
           │ (Direct 3.3V - no diode drop)
           │
    ┌──────┴──────────────────────────┐
    │                                 │
    ▼                                 ▼
┌─────────────┐               ┌───────────────┐
│  DS3231SN   │               │  MAX98357A    │
│    RTC      │               │    Audio      │
│ (3.3V VCC)  │               │  (3.3V VDD)   │
└─────────────┘               └───────────────┘
```

### Operating Scenarios

| Scenario | Battery | USB-C | Switch | What Happens |
|----------|---------|-------|--------------|
| **Normal Operation** | ✅ Inserted | ❌ Disconnected | ON | MP2359 → 3V3_BUCK powers RTC+Audio directly. D1 conducts → module gets ~3.0V |
| **USB Powered** | ❌ Removed | ✅ Connected | - | Module LDO → 3.3V. D2 conducts → 3V3_BUCK gets ~3.0V → powers RTC+Audio |
| **Both Connected** | ✅ Inserted | ✅ Connected | ON | Both rails at 3.3V → D1 OFF, D2 OFF. Battery feeds peripherals directly, USB feeds module directly. No cross-feeding. |
| **Sleep Mode** | ✅ Inserted | ❌ Disconnected | ON | MP2359 in PFM mode, ~40µA total |
| **Power OFF** | ✅ Inserted | ❌ Disconnected | OFF | 0µA draw — battery lasts years in storage |

### Why SS14 Diodes are Critical

```
D1: Battery → Module (prevents regulator conflict)

  MP2359 ──┬── 3.3V           ▶├ (SS14 D1, 0.3V drop)
         │                    │
         │               3.0V │ → Module 3V3 pin
         │                    │
         │               Module internal LDO = 3.3V
         │               D1 blocks reverse from module

D2: Module → Peripherals (USB powers RTC + Audio)

  Module 3V3 ── 3.3V          ▶├ (SS14 D2, 0.3V drop)
                                │
                           3.0V │ → RTC + Audio
                                │
  When battery ON: MP2359 3.3V > 3.0V, so battery dominates
  When USB only:   D2 feeds 3.0V to peripherals ✔️
```

### Detailed Power Flow

```
              ┌─────────────────────────────────────────────────────────┐
              │              4×AA BATTERY HOLDER                        │
              │                                                         │
              │    ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐                   │
              │    │ AA  │  │ AA  │  │ AA  │  │ AA  │  (Series)         │
              │    │1.5V │  │1.5V │  │1.5V │  │1.5V │                   │
              │    └──┬──┘  └──┬──┘  └──┬──┘  └──┬──┘                   │
              │       └───┬────┴────┬───┴────┬───┘                      │
              │           │         │        │                          │
              │        BATT+      (series) BATT-                        │
              │        (RED)       wiring  (BLACK)                      │
              └───────────┬─────────────────┬───────────────────────────┘
                          │                 │
                          ▼                 ▼
              ┌───────────────────────────────────────────────────────┐
              │                   CARRIER BOARD                       │
              │                                                       │
              │   BATT+ ──┬──────────────────────┐                    │
              │   (RED)   │                      │                    │
              │        ┌──┴──┐                   │                    │
              │        │ ON  │ SPST Switch       │                    │
              │        │ OFF │                   │                    │
              │        └──┬──┘                   │                    │
              │           │                      │                    │
              │          ═╧═ 22µF               VIN                   │
              │           │                      │                    │
              │          GND              ┌──────┴──────┐              │
              │                           │   MP2359    │              │
              │   BATT- ──────────────────┤     GND     │              │
              │  (BLACK)      │           └──────┬──────┘              │
              │               │                  │                    │
              │              GND             3.3V OUT                  │
              │                                  │                    │
              │                    ┌─────────────┼─────────────┐       │
              │                    │             │             │       │
              │                    │        SS14 ▼             │       │
              │                    │        ──▶├──             │       │
              │                    │             │ (0.3V drop) │       │
              │                    │             │             │       │
              │                    ▼             ▼             ▼       │
              │              ┌─────────┐   ┌──────────┐   ┌─────────┐  │
              │              │ DS3231  │   │ WT32S3   │   │MAX98357A│  │
              │              │  3.3V   │   │ 3V3 Pin  │   │  3.3V   │  │
              │              └─────────┘   └──────────┘   └─────────┘  │
              │                  │              │              │       │
              │                 GND            GND            GND      │
              └───────────────────────────────────────────────────────┘
```

> **Note:** RTC ve Audio direkt 3.3V alır (diyot yok), Module ise SS14 üzerinden alır.
> USB bağlandığında Module kendi LDO'sunu kullanır, pil tarafından ters akım gelmez.

### MP2359 Buck Converter (Battery Path)

```
BATTERY 6V ────┬────────────────────┐
               │                    │
              ═╧═ 22µF              │
               │                    │
              GND            ┌──────┴──────┐
                             │   MP2359    │
                             │             │
                    10µH     │  VIN    SW ─┼──┐
                ┌──█████──┐  │             │  │
                │         │  │  FB    GND ─┼──┼── GND
                │    ┌────┴──┤             │  │
                │    │       └─────────────┘  │
                │    │                        │
                │    │   Feedback Network     │
                │    │   ┌───────────────┐    │
                │    └───┤ 49.9K  16.2K  │    │
                │        │   │      │    │    │
                │        │   └──┬───┘    │    │
                │        └──────┼────────┘    │
                │               │             │
                └───────────────┴─────────────┘
                                │
                              ═╧═ 22µF
                                │
                               GND
                                │
                                └──────────────▶ 3.3V OUT
```

> **Note:** Vout = 0.81V × (1 + 49.9K/16.2K) = **3.30V**

### 4×AA Battery Holder Connection (CRITICAL)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    4×AA BATTERY HOLDER WIRING                               │
│                                                                             │
│    BATTERY HOLDER                           CARRIER PCB                     │
│                                                                             │
│    ┌─────────────────────────┐              ┌────────────────────┐          │
│    │                         │              │                    │          │
│    │  ●━━  ●━━  ●━━  ●━━    │              │   BATT+ ●──┬───────┼──▶ MP2359 VIN  │
│    │  AA   AA   AA   AA     │              │         ┌─┴─┐      │          │
│    │  ━●━  ━●━  ━●━  ━●━    │              │         │SW │      │          │
│    │                         │              │         └─┬─┘      │          │
│    └──┬───────────────┬──────┘              │        ═╧═ 22µF    │          │
│       │               │                     │         │          │          │
│       │  RED WIRE     │  BLACK WIRE         │        GND         │          │
│       │  (+) BATT+    │  (-) BATT-          │                    │          │
│       │               │                     │   BATT- ●──────────┼──▶ GND   │
│       │               │                                                     │
│       ▼               ▼                                                     │
│   ┌───────┐       ┌───────┐                                                 │
│   │ BATT+ │       │ BATT- │                                                 │
│   │ PAD   │       │ PAD   │     ← Solder pads on carrier PCB               │
│   └───────┘       └───────┘       (2.54mm pitch holes or SMD pads)         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

    VOLTAGE RANGE:
    ┌──────────────────┬─────────────────┐
    │ Battery State    │ Voltage         │
    ├──────────────────┼─────────────────┤
    │ Fresh Alkaline   │ 6.0V - 6.4V     │
    │ Normal Use       │ 4.8V - 6.0V     │
    │ Depleted         │ 4.0V - 4.8V     │
    │ MP2359 Min Input │ 4.5V            │
    └──────────────────┴─────────────────┘
```

### PCB Pads for Battery Holder

```
    ┌─────────────────────────────────────────────┐
    │              CARRIER PCB                    │
    │                                             │
    │   ┌─────┐                      ┌─────┐      │
    │   │BATT+│ ← Red wire           │BATT-│      │
    │   │ (+) │   from holder        │ (-) │      │
    │   └──┬──┘                      └──┬──┘      │
    │      │                            │         │
    │      │   ┌────────────────────┐   │         │
    │      │   │  SPDT Slide Switch │   │         │
    │      └──▶│  (SS-12D00G4-4MM)  │   │         │
    │          └─────────┬──────────┘   │         │
    │                    │              │         │
    │          ┌─────────▼──────────┐   │         │
    │          │    22µF INPUT CAP  │◀──┘         │
    │          │      │    │        │             │
    │          │     VIN  GND       │             │
    │          │   ┌──────────┐     │             │
    │          │   │  MP2359  │     │             │
    │          │   │  Buck    │     │             │
    │          │   └────┬─────┘     │             │
    │          │        │ 3.3V     │              │
    │          └────────┼──────────┘              │
    │                   ▼                         │
    │           To WT32S3-28S 3V3 pin             │
    │           To DS3231 VCC                     │
    │           To MAX98357A VDD                  │
    │           To Button pull-up (GPIO8)         │
    │                                             │
    └─────────────────────────────────────────────┘
```

---

## Audio System (MAX98357A)

### MAX98357AETE+T Wiring

```
                              MAX98357AETE+T
                           ┌─────────────────────┐
                           │                     │
   3.3V ──────────────────┤ VDD               1 │
                           │                     │
   GND ───────────────────┤ GND               2 │
                           │                     │
   GPIO1 (SD/Enable) ─────┤ SD_MODE           3 │  (HIGH = Stereo mix, LOW = Shutdown)
                           │                     │
   NC (Internal pull-down) │ GAIN             4 │  (GND = 9dB, VDD = 15dB)
                           │                     │
   GPIO3 (I2S BCLK) ──────┤ BCLK              5 │
                           │                     │
   GPIO2 (I2S LRCK) ──────┤ LRCLK             6 │
                           │                     │
   GPIO4 (I2S DIN) ───────┤ DIN               7 │
                           │                     │
                           │ OUTP ─────────────┬─┼──▶ Speaker (+)
                           │                   │ │
                           │ OUTN ────────┬────┼─┼──▶ Speaker (-)
                           │              │    │ │
                           └──────────────┼────┼─┘
                                          │    │
                                        1µF  1µF    (Optional output filtering)
                                          │    │
                                         GND  GND
```

### MAX98357A Decoupling (Critical for Audio Quality)

```
              VDD Pin
                │
      ┌─────────┼─────────┐
      │         │         │
     ═╧═       ═╧═       ═╧═
    10µF      100nF      10pF    ← Place as close as possible to VDD pin
      │         │         │
      └─────────┴─────────┘
                │
               GND
```

### I2S Configuration

| Parameter | Value |
|-----------|-------|
| Sample Rate | 44100 Hz |
| Bits per Sample | 16-bit |
| Channels | Mono (L+R mixed) |
| Format | I2S Philips |
| BCLK Frequency | 44100 × 16 × 2 = 1.41 MHz |

---

## Physical Button (Wake/Mute/Settings)

### Button Circuit

```
        3.3V (3V3_BUCK)
          │
         10K  (pull-up)
          │
          ├─────────────────▶ GPIO8
          │
        ┌─┴─┐
        │   │  Tactile
        │ ○ │  Switch
        │   │  (6×6mm)
        └─┬─┘
          │
         GND
```

> **Note:** Button is active LOW (pressed = 0, released = 1).  
> GPIO8 is used as wake source from deep sleep: `esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0)`

### Button Behavior Map

| Context | Press | Action |
|---------|-------|--------|
| Deep sleep | Any press | Wake device, turn on display |
| Adhan playing | Short press | Stop/mute adhan |
| Normal (screen on) | Short press | Toggle mute |
| Normal (screen on) | Long press (3s) | Open settings / WiFi reconnect |
| Screen off (light sleep) | Any press | Wake display |

---

## Power Switch (ON/OFF)

### Switch Circuit

```
    4×AA BATTERIES          SPDT Slide Switch
         │                  (SS-12D00G4-4MM)
      BATT+ (Red)        ┌──────────┐
         │               │   ┌──┐   │
         └─────────────┤ 1 │  │ 2 ├───▶ MP2359 VIN
                          │   └──┘   │
                          └──────────┘

    ON position:  Pin 1 connected to Pin 2 → Power flows
    OFF position: Pin 1 disconnected      → 0µA drain
```

> **Placement:** Side of enclosure, easily accessible.  
> **Rating:** Must handle >200mA at 6V (SS-12D00 rated 0.3A @ 6V DC = adequate)

---

## RTC Module (DS3231SN)

### DS3231SN Wiring

```
                           DS3231SN (SO-16W)
                    ┌──────────────────────────┐
                    │                          │
   NC ─────────────┤ 1  32KHz        VCC  16 ├────── 3.3V
                    │                          │
   NC ─────────────┤ 2  INT/SQW      VBAT 15 ├────── CR1220 (+)
                    │                          │        │
   GPIO16 ─────────┤ 3  RST           N/C  14 ├── NC   └── 100K ── GND (trickle disable)
   (optional)       │                          │
                    │ 4  N/C          GND  13 ├────── GND, CR1220 (-)
                    │                          │
                    │ 5  N/C          N/C  12 ├── NC
                    │                          │
                    │ 6  N/C          N/C  11 ├── NC
                    │                          │
                    │ 7  N/C          N/C  10 ├── NC
                    │                          │
   GPIO15 ── 4.7K ─┤ 8  SDA          N/C   9 ├── NC
       │           │                          │
   GPIO7 ─── 4.7K ─┤               SCL ───────┤
       │           └──────────────────────────┘
       │
   To 3.3V (I2C pull-ups)
```

### I2C Configuration

| Parameter | Value |
|-----------|-------|
| I2C Address | 0x68 |
| I2C Speed | 400 kHz (Fast Mode) |
| Pull-up Resistors | 4.7kΩ to 3.3V |
| SDA | GPIO15 |
| SCL | GPIO7 |

---

## Carrier Board Pin Mapping

### Complete Wiring List

| WT32S3 Pin | GPIO | Function | Destination | Notes |
|------------|------|----------|-------------|-------|
| Row1-1 | 3V3 | Power | All ICs VCC | 3.3V rail |
| Row1-2 | GPIO7 | I2C SCL | DS3231 SCL | 4.7K pull-up |
| Row1-3 | GPIO15 | I2C SDA | DS3231 SDA | 4.7K pull-up |
| Row1-4 | GPIO16 | RTC INT | DS3231 SQW | Optional wake |
| Row1-7 | GPIO8 | BTN (Wake/Mute) | Physical Button | 10K pull-up, active LOW |
| Row1-8 | GPIO3 | I2S BCLK | MAX98357 BCLK | Bit clock |
| Row1-10 | GND | Ground | All ICs GND | Common ground |
| Row2-11 | 5V | Power In | From 3.3V rail | Module power |
| Row2-12 | GPIO6 | USB_DETECT | USB 5V Divider | HIGH=USB |
| Row2-14 | GPIO4 | I2S DIN | MAX98357 DIN | Audio data |
| Row2-15 | GPIO2 | I2S LRCK | MAX98357 LRCK | Word select |
| Row2-16 | GPIO1 | AMP_EN | MAX98357 SD | Shutdown ctrl |
| Row2-20 | GND | Ground | All ICs GND | Common ground |

### Schematic Net List (for EasyEDA/KiCad Import)

```
NET LIST - Carrier Base Board for WT32S3-28S PLUS (4×AA Version with USB Isolation)
====================================================================================

Power Nets:
-----------
NET: VBAT (4.5V-6.4V from batteries)
  - 4×AA Battery holder RED wire (+) → BATT+ pad
  - SPDT slide switch SS-12D00G4-4MM (power ON/OFF)
  - 22µF input capacitor (+)
  - MP2359 VIN pin

NET: 3V3_BUCK (MP2359 output + USB reverse feed)
  - MP2359 output (via inductor)
  - 22µF output cap (+)
  - SS14 D1 anode (battery → module isolation)
  - SS14 D2 cathode (module → peripherals feed)
  - DS3231 VCC
  - MAX98357 VDD
  - I2C Pull-ups (2× 4.7K)
  - Button pull-up (10K)

NET: 3V3_MODULE (module's 3V3 pin)
  - SS14 D1 cathode (receives battery power)
  - SS14 D2 anode (outputs USB power to peripherals)
  - WT32S3-28S 3V3 pin (Row1-1)

NET: GND
  - 4×AA Battery holder BLACK wire (-) → BATT- pad
  - MP2359 GND pin
  - All capacitors (-)
  - WT32S3-28S GND (Row1-10, Row2-20)
  - DS3231 GND
  - MAX98357 GND
  - SS14 not connected to GND (diode in series, not to ground)

Signal Nets:
------------
NET: I2C_SDA
  - WT32S3 GPIO15 (Row1-3)
  - DS3231 SDA (pin 8)
  - 4.7K pull-up to 3V3

NET: I2C_SCL
  - WT32S3 GPIO7 (Row1-2)
  - DS3231 SCL
  - 4.7K pull-up to 3V3

NET: I2S_BCLK
  - WT32S3 GPIO3 (Row1-8)
  - MAX98357 BCLK (pin 5)

NET: I2S_LRCK
  - WT32S3 GPIO2 (Row2-15)
  - MAX98357 LRCLK (pin 6)

NET: I2S_DIN
  - WT32S3 GPIO4 (Row2-14)
  - MAX98357 DIN (pin 7)

NET: AMP_ENABLE
  - WT32S3 GPIO1 (Row2-16)
  - MAX98357 SD_MODE (pin 3)

NET: RTC_INT (optional)
  - WT32S3 GPIO16 (Row1-4)
  - DS3231 INT/SQW (pin 2)
  - 10K pull-up to 3V3

NET: BTN_WAKE
  - WT32S3 GPIO8 (Row1-7)
  - 10K pull-up to 3V3_BUCK
  - Tactile switch to GND
  - Active LOW (pressed = 0)
  - Deep sleep wake source

NET: SPKR_P
  - MAX98357 OUTP
  - Speaker (+)

NET: SPKR_N
  - MAX98357 OUTN
  - Speaker (-)

Internal Buck Converter Nets:
-----------------------------
NET: SW (switching node)
  - MP2359 SW pin
  - 10µH inductor (one end)
  - D3 Cathode (SS14 freewheeling/catch diode)
  - Bootstrap cap C5 (one end)

NET: BS (bootstrap)
  - MP2359 BST pin
  - Bootstrap cap C5 (other end)
  
NET: FB (feedback)
  - MP2359 FB pin
  - 49.9K resistor to 3V3_BUCK
  - 16.2K resistor to GND
```

---

## PCB Layout Guidelines (JLCPCB)

### Trace Width Recommendations

| Net | Current | Trace Width | Layer |
|-----|---------|-------------|-------|
| VBAT (6V) | 200mA peak | 0.5mm (20mil) | Top |
| 3V3 Rail | 500mA | 0.4mm (16mil) | Top |
| I2S Signals | <10mA | 0.25mm (10mil) | Top |
| I2C Signals | <1mA | 0.2mm (8mil) | Top |
| GND | Return | Polygon fill | Both |

### Critical Layout Rules

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       EMI/EMC GUIDELINES                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. GROUND PLANE                                                        │
│     ├─ Bottom layer: Solid GND plane (no splits under audio)           │
│     ├─ Top layer: GND pour around components                           │
│     └─ Via stitching: Every 5mm around board edge                      │
│                                                                         │
│  2. POWER SECTION (MP2359 Buck Converter) — per datasheet Fig.2        │
│     ├─ Place near battery connector input                              │
│     ├─ Minimize loop area: C1(+) → U1.IN → U1.SW → D3 → GND → C1(-)  │
│     ├─ D3 (Schottky): SW-to-GND path as short and wide as possible     │
│     ├─ Short, wide traces for SW node (high di/dt switching)           │
│     ├─ Route SW AWAY from FB — noisy SW corrupts feedback sensing      │
│     ├─ Input cap C1 within 3mm of IN pin, output cap C2 near L1       │
│     ├─ Keep feedback resistors R4/R5 close to FB pin (Pin 3)           │
│     ├─ Inductor placement minimizes SW loop area                       │
│     └─ Connect IN, SW, GND pads to large copper areas for cooling      │
│                                                                         │
│  3. USB POWER PATH (SS14 Diodes)                                       │
│     ├─ Place SS14 diodes near module USB-C output                      │
│     ├─ Short traces from diode cathodes to 3.3V rail                   │
│     ├─ D1: Battery path isolation (after buck converter)               │
│     └─ D2: USB path isolation (from module VBUS)                       │
│                                                                         │
│  4. AUDIO SECTION (MAX98357A)                                          │
│     ├─ Place away from switching regulators                            │
│     ├─ Decoupling caps within 2mm of VDD pin                           │
│     ├─ Keep I2S traces parallel, equal length (±2mm)                   │
│     ├─ Avoid routing I2S over gaps in ground plane                     │
│     └─ Speaker traces can be wider (0.5mm) to reduce resistance        │
│                                                                         │
│  5. I2C SECTION (DS3231)                                               │
│     ├─ Keep SDA/SCL traces short (<30mm)                               │
│     ├─ Pull-ups close to RTC IC                                        │
│     └─ Place backup battery holder on bottom if space tight            │
│                                                                         │
│  6. MODULE CONNECTION                                                   │
│     ├─ Use 2×10 pin header (2.54mm pitch)                              │
│     ├─ Match header footprint to WT32S3 exactly                        │
│     └─ Consider castellated holes for direct soldering                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Component Placement (Top View)

```
┌────────────────────────────────────────────────────────────────────────┐
│                           70mm                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                                                                 │   │
│  │             WT32S3-28S PLUS MODULE                              │   │
│  │               (Mounted on headers)                              │   │
│  │           77 × 47mm with 2.8" display                           │   │
│  │                                                                 │   │
│  └───────────────────────────┬─────────────────────────────────────┘   │
│                              │                                         │
│  ┌────────┐  ┌────────────┐  │  ┌─────────────┐  ┌──────────────┐     │
│  │        │  │   MP2359   │  │  │             │  │              │     │
│  │ USB-C  │  │   Buck     │  │  │  DS3231SN   │  │  MAX98357A   │     │
│  │        │  │   + Diodes │  │  │  + CR1220   │  │              │     │
│  │        │  └────────────┘  │  │             │  │              │     │
│  └────────┘  ┌────────────┐  │  └─────────────┘  └──────────────┘     │
│              │ SPST Switch│  │                                        │
│              │  (ON/OFF)  │  │                     ┌────────────────┐ │
│              └────────────┘  │  ┌─────────┐        │    SPEAKER     │ │
│                              │  │  BTN1   │        │    CONNECTOR   │ │
│                              │  │  (6×6)  │        └────────────────┘ │
│                              ▼  └─────────┘                           │
│                         4×AA Battery                                  │
│                          Connector                                    │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
◄──────────────────────── 55mm ──────────────────────────────────────────►

BOTTOM SIDE: 
  - 4×AA Battery Holder (underneath module)
  - CR1220 holder (if space needed)
```

---

## Manufacturing Files

### JLCPCB BOM Format

```csv
Comment,Designator,Footprint,LCSC Part #
"Buck Converter",U1,SOT-23-6,C52205265
"I2S Audio Amplifier",U2,QFN-16,C910544
"Precision RTC",U3,SO-16W,C9866
"Schottky Diode",D1 D2 D3,SMA,C2480
"10K Resistor",R1,0402,C25744
"4.7K Resistor",R2 R3,0402,C25900
"49.9K Resistor 1%",R4,0402,C25905⚠️
"16.2K Resistor 1%",R5,0402,C25764⚠️
"22uF Cap",C1 C2,0805,C45783
"10uF Cap",C3,0402,C15525
"100nF Cap",C4 C5,0402,C1525
"10uH 2.2A Inductor",L1,1210,XRTC322512S100MBCA⚠️
"CR1220 Holder",BT1,SMD,C5365932
"SPDT Slide Switch",SW1,SMD-3P,C50377150
"Tactile Switch",SW2,SMD-4P,C2939240
```

### CPL (Centroid) File Format

```csv
Designator,Mid X,Mid Y,Layer,Rotation
U1,15,12,T,0
U2,50,15,T,0
U3,50,28,T,0
D1,20,8,T,0
D2,28,8,T,0
L1,18,16,T,0
SW1,5,25,T,0
SW2,60,8,T,0
...
```

---

## Battery Life Calculation

**Usage Scenario:** 6 min/day adhan (screen OFF) + 7×15sec touch interactions

#### Component Current Draw (from datasheets)

| Component | Active | Sleep/Shutdown | Source |
|-----------|--------|----------------|--------|
| ESP32-S3R8 | 40-50mA | 40µA (PSRAM) | Espressif Datasheet |
| WT32S3 Display | 70mA (backlight) | 0 (IO45 LOW) | ST7789V Datasheet |
| MAX98357A @ 60% vol | 85mA @ 3.3V | 0.6µA (SD=LOW) | MAX98357A Datasheet |
| DS3231 RTC | 0.2mA | 0.2mA | DS3231 Datasheet |

> **Note:** MAX98357A @ 3.3V max output = V²/(2×R) = 3.3²/(2×8) = **0.68W**. At 60% volume: 0.25W output.

#### Daily Energy Consumption

| State | Current (3.3V) | Duration | Energy |
|-------|----------------|----------|--------|
| Adhan (ESP32 + audio, screen OFF) | 135mA | 6 min (0.1h) | 13.5 mAh |
| Display ON (touch wake) | 120mA | 1.75 min (0.029h) | 3.5 mAh |
| Deep Sleep (ESP32 + RTC) | 40µA | 23.87 h | 1.0 mAh |
| **Subtotal** | | | **18 mAh** |
| Buck converter loss (90% eff.) | ×1.11 | | +2 mAh |
| **Total per day** | | | **~20 mAh** |

#### Battery Life Estimate

```
4×AA Batteries (2400 mAh) ÷ 20 mAh/day = 120 days ≈ 4 months
```

| Usage Pattern | Battery Life |
|---------------|--------------|
| Optimized (6 min ezan, 2 min screen) | **~4 months** |
| Moderate (+ 10 min screen/day) | ~2.5 months |
| Heavy (+ 30 min screen/day) | ~1 month |

#### Power Optimization Requirements

For maximum battery life, firmware MUST implement:

1. **ESP32-S3 Deep Sleep**: Use `esp_deep_sleep_start()` between prayer times
2. **MAX98357A Shutdown**: Set SD/EN pin LOW when not playing audio
3. **Backlight Control**: Set IO45 LOW when screen not needed
4. **RTC Wake**: Use DS3231 alarm to wake for prayer times
5. **Button Wake**: Use `esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0)` for physical button

```cpp
// Example power optimization
void enterDeepSleep() {
    digitalWrite(AMP_EN_PIN, LOW);      // Shutdown amplifier (~0.6µA)
    digitalWrite(BACKLIGHT_PIN, LOW);   // Turn off display backlight
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0);  // Wake on button press (active LOW)
    // Configure RTC alarm for next prayer
    esp_deep_sleep_start();             // ESP32-S3 enters deep sleep
}
```

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-14 | Initial design (WT32S3-28S PLUS module-based) |
| 1.1 | 2026-02-07 | Changed from Li-ion to 4×AA batteries |
| 1.2 | 2026-02-07 | Added detailed battery life calculation with datasheet values (~4 months) |
| 1.3 | 2026-02-07 | Simplified - removed carrier USB-C (uses module's USB-C for programming) |
| 1.4 | 2026-02-07 | Added SS14 USB isolation - prevents reverse current when USB+battery connected |
| 1.5 | 2026-02-09 | Added SPST power switch (ON/OFF) and physical button (GPIO8) for wake/mute/settings |
| 1.6 | 2026-02-09 | Added 2nd SS14 diode (D2) so USB-C can fully power device (RTC + Audio) without batteries |
| 1.7 | 2026-02-14 | Fixed MP2359 feedback resistors (R4: 18K→49.9K 1%, R5: 3.9K→16.2K 1%), corrected Vref from 0.6V to 0.81V per datasheet |
| 1.8 | 2026-02-14 | Upgraded L1 inductor from 0805/C1046 (~0.4A) to 1210/XRTC322512S100MBCA (2.2A Isat) for max volume support |
