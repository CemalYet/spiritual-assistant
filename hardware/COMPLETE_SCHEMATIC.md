# Spiritual Assistant — Complete Hardware Schematic & Wiring Guide

> **Battery-Powered Smart Adhan Desk Clock with Integrated Touch Display**
> WT32S3-28S PLUS Module | ESP32-S3 | 4×AA Batteries | 2.8" IPS Capacitive Touch | ~4 months battery life

> **This single document is your ONLY reference for the entire hardware design.**
> Every spec, every pin, every wire, every net label is documented below.

---

## Table of Contents

1. [Overview](#overview)
2. [Bill of Materials](#bill-of-materials)
3. [System Architecture](#system-architecture)
4. [WT32S3-28S PLUS Module](#wt32s3-28s-plus-module)
5. [EasyEDA: Place Components](#easyeda-place-components)
6. [Wiring — Section A: Battery Connector (J2)](#a-battery-connector-j2--c8269)
7. [Wiring — Section B: Power Slide Switch (SW1)](#b-power-slide-switch-sw1--c50377150)
8. [Wiring — Section C: Buck Converter (U1)](#c-buck-converter-u1--mp2359dj--c52205265)
9. [Wiring — Section D: Diode Isolation (D1, D2)](#d-diode-isolation-d1-d2--c2480)
10. [Wiring — Section E: RTC (U3)](#e-rtc-u3--ds3231sn--c9866)
11. [Wiring — Section F: Audio Amp (U2)](#f-audio-amp-u2--max98357aetet--c910544)
12. [Wiring — Section G: Tactile Button (SW2)](#g-tactile-button-sw2--c2939240)
13. [Wiring — Section H: Speaker Connector (J3)](#h-speaker-connector-j3--c8269)
14. [Wiring — Section I: Module Header (J1)](#i-module-header-j1--2x10-pin--c5383109)
15. [Complete Net Label Reference](#complete-net-label-reference)
16. [65-Connection Checklist](#65-connection-checklist)
17. [Complete Power Flow Diagram](#complete-power-flow-diagram)
18. [Power Architecture & USB Isolation](#power-architecture--usb-isolation)
19. [Battery Life Calculation](#battery-life-calculation)
20. [PCB Layout Guidelines (JLCPCB)](#pcb-layout-guidelines-jlcpcb)
21. [Manufacturing Files](#manufacturing-files)
22. [EasyEDA: ERC, PCB Conversion & Ordering](#easyeda-erc-pcb-conversion--ordering)
23. [Notes & Warnings](#notes--warnings)

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
| **Estimated Cost** | ~€16.41 |

---

## Bill of Materials

| Part | LCSC # | Description | Package | Type | Price |
|------|--------|-------------|---------|------|-------|
| WT32S3-28S PLUS | — | ESP32-S3 + 2.8" IPS Touch Module | Module | — | €12.00 |
| MP2359DJ-LF-Z-TP | C52205265 | Buck Converter IC | SOT-23-6 | Extended | €0.16 |
| MAX98357AETE+T | C910544 | I2S 3W Audio Amplifier | QFN-16 | Extended | €0.80 |
| DS3231SN | C9866 | Precision RTC ±2ppm | SO-16W | Extended | €2.50 |
| CR1220 Holder | C5365932 | KH-BS1220-2-SMT RTC Backup Battery | SMD | Extended | €0.08 |
| SS14 Schottky ×3 | C2480 | 1A 40V diode (power isolation + buck catch) | SMA | Basic | €0.15 |
| SPDT Slide Switch | C50377150 | SS12D00G6 ON/OFF power switch | SMD-3P | Extended | €0.02 |
| Tactile Switch | C2939240 | TS-1187F-015E HOOYA (wake/mute) | SMD-4P | Extended | €0.03 |
| 10K Resistor | C25744 | Pull-up for button | 0402 | Basic | €0.01 |
| 4.7K Resistor ×2 | C25900 | I2C pull-ups (SDA/SCL) | 0402 | Basic | €0.01 |
| 49.9K Resistor (1%) | C25905 ⚠️ | MP2359 feedback (high side) | 0402 | Extended | €0.01 |
| 16.2K Resistor (1%) | C25764 ⚠️ | MP2359 feedback (low side) | 0402 | Extended | €0.01 |
| 22µF Cap ×2 | C45783 | Input/output caps for buck | 0805 | Basic | €0.05 |
| 10µF Cap | C15525 | MAX98357A decoupling | 0402 | Basic | €0.02 |
| 100nF Cap ×2 | C1525 | Decoupling / bootstrap | 0402 | Basic | €0.01 |
| 10µH 2.2A Inductor | XRTC322512S100MBCA⚠️ | Power inductor for buck (molded, 210mΩ) | 1210 | Extended | €0.10 |
| 2x10 Pin Header | C5383109 | Module carrier connector | 2.54mm | — | — |
| 2-pin Screw Terminal ×2 | C8269 | Battery + Speaker connectors | 5.0mm | — | — |
| Speaker 3W 4Ω | — | 28mm Mini Speaker | — | — | €0.60 |
| 4×AA Holder | — | 4×AA Battery Holder (buy separately) | — | — | €0.40 |
| PCB 2-layer | — | 65×50mm Carrier Board | — | — | €2.00 |
| **TOTAL** | | | | | **~€16.41** |

### Designator Reference

| Designator | Component |
|------------|-----------|
| U1 | MP2359DJ Buck Converter |
| U2 | MAX98357AETE+T Audio Amp |
| U3 | DS3231SN RTC |
| D1 | SS14 Schottky Diode (battery → module) |
| D2 | SS14 Schottky Diode (module → peripherals) |
| D3 | SS14 Schottky Diode (buck freewheeling / catch) |
| R1 | 10K Resistor (button pull-up) |
| R2 | 4.7K Resistor (I2C SDA pull-up) |
| R3 | 4.7K Resistor (I2C SCL pull-up) |
| R4 | 49.9K Resistor 1% (FB divider high side) |
| R5 | 16.2K Resistor 1% (FB divider low side) |
| C1 | 22µF Cap (buck input) |
| C2 | 22µF Cap (buck output) |
| C3 | 10µF Cap (audio decoupling) |
| C4 | 100nF Cap (audio decoupling) |
| C5 | 100nF Cap (buck bootstrap) |
| L1 | 10µH 2.2A Inductor (buck, 1210) |
| BT1 | CR1220 Battery Holder (RTC backup) |
| SW1 | SS12D00G6 Slide Switch (power ON/OFF) |
| SW2 | HOOYA TS-1187F-015E Tactile Switch (wake/mute) |
| J1 | 2x10 Pin Header (module connector) |
| J2 | 2-pin Screw Terminal (battery) |
| J3 | 2-pin Screw Terminal (speaker) |

### JLCPCB Part Classification

| Category | Parts | Notes |
|----------|-------|-------|
| **Basic** | SS14, Passives (C1-C5, R1-R3) | No extra fee |
| **Extended** | MP2359, MAX98357A, DS3231SN, CR1220 Holder, Slide Switch, Tactile Switch, R4, R5, L1 | +€3 per unique extended part |

---

## System Architecture

```
┌───────────────────────────────────────────────────────────────────────────┐
│                        SYSTEM BLOCK DIAGRAM                               │
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
                       │ OFF │  (SS12D00G6)
                       └──┬──┘        │
                          │           │
                          ▼           ▼
              ┌───────────────────────────────────────────────────────┐
              │                   CARRIER PCB                         │
              │                                                       │
              │   ┌──────────────────┐                                │
              │   │     MP2359       │                                │
              │   │   Buck Conv.     │                                │
              │   │   6V → 3.3V      │                                │
              │   │    η = 90%       │                                │
              │   └────────┬─────────┘                                │
              │            │                                          │
              │            ▼                                          │
              │     ┌─────────────┐                                   │
              │     │  3.3V RAIL  │                                   │
              │     └──────┬──────┘                                   │
              │            │                                          │
              │   ┌────────┼────────┐                                 │
              │   │        │        │                                 │
              │   ▼        ▼        ▼                                 │
              │  RTC    Module   Audio                                │
              │                                                       │
              │   ┌──────────────────┐                                │
              │   │  Physical Button │                                │
              │   │  GPIO8 (Wake/    │                                │
              │   │   Mute/Settings) │                                │
              │   └──────────────────┘                                │
              └───────────────────────────────────────────────────────┘
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
│  └───────────────────────────────┘  │
│  Expansion Header (2×10 pins)       │
└─────────────────────────────────────┘
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
│ 4   │ GPIO16 │ RTC INT (optional)                 │
│ 5   │ GPIO17 │ Available                          │
│ 6   │ GPIO18 │ Available                          │
│ 7   │ GPIO8  │ Button (Wake/Mute)                 │
│ 8   │ GPIO3  │ I2S BCLK → MAX98357 BCLK           │
│ 9   │ GPIO46 │ Available                          │
│ 10  │ GND    │ Ground                             │
└─────┴────────┴────────────────────────────────────┘

Pin Map (Row 2 - Bottom):
┌─────┬────────┬────────────────────────────────────┐
│ Pin │ Name   │ Function                           │
├─────┼────────┼────────────────────────────────────┤
│ 11  │ 5V     │ 5V Input — NOT USED (leave NC)     │
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

## EasyEDA: Place Components

In EasyEDA: Press **"L"** → Search by LCSC# → Click **"Place"**

| LCSC # | Part | Designator | Qty |
|--------|------|------------|-----|
| C52205265 | MP2359DJ Buck Converter (SOT-23-6) | U1 | 1 |
| C910544 | MAX98357AETE+T Audio Amp (QFN-16) | U2 | 1 |
| C9866 | DS3231SN RTC (SO-16W) | U3 | 1 |
| C2480 | SS14 Schottky Diode (SMA) | D1 | 1 |
| C2480 | SS14 Schottky Diode (SMA) | D2 | 1 |
| C2480 | SS14 Schottky Diode (SMA) | D3 | 1 |
| C25744 | 10K Resistor 0402 | R1 | 1 |
| C25900 | 4.7K Resistor 0402 | R2 | 1 |
| C25900 | 4.7K Resistor 0402 | R3 | 1 |
| C25905 ⚠️ | 49.9K Resistor 0402 1% | R4 | 1 |
| C25764 ⚠️ | 16.2K Resistor 0402 1% | R5 | 1 |
| C45783 | 22uF Cap 0805 | C1 | 1 |
| C45783 | 22uF Cap 0805 | C2 | 1 |
| C15525 | 10uF Cap 0402 | C3 | 1 |
| C1525 | 100nF Cap 0402 | C4 | 1 |
| C1525 | 100nF Cap 0402 | C5 | 1 |
| XRTC322512S100MBCA⚠️ | 10uH 2.2A Inductor 1210 | L1 | 1 |
| C5365932 | CR1220 Battery Holder | BT1 | 1 |
| C50377150 | SS12D00G6 Slide Switch (3 pin) | SW1 | 1 |
| C2939240 | HOOYA TS-1187F-015E Tactile Switch (4 pin SPST) | SW2 | 1 |
| C5383109 | 2x10 Pin Header 2.54mm | J1 | 1 |
| C8269 | 2-pin Screw Terminal 5.0mm | J2 | 1 |
| C8269 | 2-pin Screw Terminal 5.0mm | J3 | 1 |

**After placing:** Rename each designator in the right panel to match the table above.

---

## Wiring — Section A through I

Wire the schematic following each section in order.

---

### A. BATTERY CONNECTOR (J2) — C8269

**2-pin screw terminal. Battery holder wires solder/screw here.**

```
         J2 (2-pin Screw Terminal)
        ┌──────────────────────┐
        │                      │
        │  Pin 1       Pin 2   │
        │  (BATT+)    (BATT-)  │
        └───┬──────────┬───────┘
            │          │
            │          └──────────→ GND
            │
            └─────────────────────→ SW1.Pin1 (wire directly)
```

| J2 Pin | Connect To | Net Label |
|--------|-----------|-----------|
| **Pin 1** | SW1 Pin 1 (direct wire) | — |
| **Pin 2** | GND | **GND** |

---

### B. POWER SLIDE SWITCH (SW1) — C50377150

**SS12D00G6 — 3-pin SPDT slide switch. Controls ON/OFF power.**

```
             SW1 (SS12D00G6)
            ┌──────────────────┐
            │    ◄ SLIDER ►    │
            └──┬─────┬─────┬───┘
               │     │     │
             Pin 1  Pin 2  Pin 3

   Pin 1 = One throw (connects to battery)
   Pin 2 = Common / middle (output = VIN)
   Pin 3 = Other throw (leave NC)
```

**Wiring:**

| SW1 Pin | Connect To | Net Label |
|---------|-----------|-----------|
| **Pin 1** | J2 Pin 1 (direct wire from battery +) | — |
| **Pin 2** | VIN net (middle pin = common) | **VIN** |
| **Pin 3** | Leave unconnected (NC) | — |

**How it works:**
- Slider to ON → Pin 1 connects to Pin 2 → battery flows to VIN → device ON
- Slider to OFF → Pin 1 disconnects → 0µA drain → device OFF

---

### C. BUCK CONVERTER (U1 — MP2359DJ) — C52205265

**SOT-23-6 package. Converts 6V battery to 3.3V.**

```
                              MP2359DJ (U1)
                           ┌─────────────────┐
                           │                 │
  SW1.Pin2 (VIN) ──┬───┬───┤ 5.IN            │
                   │   │   │                 │
                  ═╧═  │   │ 2.GND ──────────┼───→ GND
                  C1   │   │                 │
                (22µF) │   │ 3.FB ───────────┼───┬─── R4 (49.9K) ──→ 3V3_BUCK net
                   │   │   │                 │   │
                  GND  │   │                 │   └─── R5 (16.2K) ──→ GND
                       │   │                 │
                       └───┤ 4.EN            │  (EN tied to IN = always enabled)
                           │                 │
                           │ 1.BS ───────────┼───┐
                           │                 │   │ C5 (100nF)
                           │ 6.SW ───────────┼───┴──┬─→ L1 ──→ 3V3_BUCK ──┬── C2 (22µF) ── GND
                           │                 │      │          (3.3V out)  │
                           └─────────────────┘      └── D3.K (cathode)  (output cap)
                                                        │▶|
                                                   D3.A (anode) ──→ GND
                                                   (SS14 freewheeling diode — REQUIRED)
```

**U1 pin-by-pin connection table:**

| U1 Pin # | Pin Name | Connect To | Net Label |
|----------|----------|-----------|-----------|
| **1** | BST | C5 one side (other side goes to Pin 6 SW) | — |
| **2** | GND | Ground | **GND** |
| **3** | FB | Junction of R4 and R5 (see below) | — |
| **4** | EN | Same wire as Pin 5 (IN) — tie together | **VIN** |
| **5** | IN | SW1.Pin2 + C1 one side + U1.Pin4 (EN) | **VIN** |
| **6** | SW | C5 other side + L1 one side + D3.Cathode | — |

**Surrounding components wired to U1:**

```
COMPONENT        PIN A CONNECTS TO          PIN B CONNECTS TO
─────────        ──────────────────          ──────────────────
C1  (22µF)  :    Pin A → VIN net            Pin B → GND
C5  (100nF) :    Pin A → U1.Pin1 (BST)      Pin B → U1.Pin6 (SW)
L1  (10µH)  :    Pin A → U1.Pin6 (SW)       Pin B → 3V3_BUCK net
C2  (22µF)  :    Pin A → 3V3_BUCK net       Pin B → GND
R4  (49.9K) :    Pin A → 3V3_BUCK net       Pin B → U1.Pin3 (FB) junction
R5  (16.2K) :    Pin A → U1.Pin3 (FB) junction   Pin B → GND
D3  (SS14)  :    K (Cathode) → U1.Pin6 (SW)      A (Anode) → GND
```

**Feedback voltage divider detail:**

```
  3V3_BUCK ──→ R4 (49.9K) ──┬──→ U1.Pin3 (FB)
                             │
                R5 (16.2K) ──┘
                    │
                   GND

  Result: Vout = 0.81V × (1 + 49.9K/16.2K) = 0.81V × 4.08 = 3.30V ✓
  (per MP2359 datasheet Table 1: R1=49.9K, R2=16.2K for 3.3V output)
```

**Complete buck converter circuit:**

```
           C1 (22µF)
  VIN ──┬──┤├──── GND
        │
        │         MP2359 (U1)
        │      ┌──────────────┐
        ├──────┤ 5.IN    6.SW ├──┬──── L1 ────── 3V3_BUCK ──┬── C2 (22µF) ── GND
        │      │              │  │                           │
        └──────┤ 4.EN    1.BS ├──┤── C5 (100nF)             ├── R4 (49.9K) ─┐
               │              │                              │               │
               │ 2.GND  3.FB ├──┼───────────────────────────┤              ├── FB
               │    │         │  │                           │              │
               └────┼─────────┘  │                           │   R5 (16.2K) ─┘
                    │            │                           │       │
                   GND       GND─▶├─┘  D3 (SS14)             │      GND
                             (catch/freewheeling diode)      │
                                                      (to D1, U3, U2, R1, R2, R3)
```

---

### D. DIODE ISOLATION (D1, D2) — C2480

**SS14 Schottky diodes. Two used for power path isolation between battery and USB.**

**IMPORTANT: Diode symbol in EasyEDA shows A (Anode) and K (Cathode).**
Current flows: A → K (like an arrow pointing from A to K).

```
  SS14 Diode Symbol:
      A ──▶|── K
     (Anode)  (Cathode)
```

**D1 — Battery → Module (isolates battery from USB):**

```
  3V3_BUCK ──→ D1.A (Anode) ──▶|── D1.K (Cathode) ──→ 3V3_MODULE net
```

| D1 Pin | Connect To | Net Label |
|--------|-----------|-----------|
| **A (Anode)** | 3V3_BUCK (L1 output / C2 / R4 / U3.VCC / U2.VDD) | **3V3_BUCK** |
| **K (Cathode)** | 3V3_MODULE (J1.Pin1 / D2.Anode) | **3V3_MODULE** |

**D2 — Module → Peripherals (USB powers RTC+Audio when no battery):**

```
  3V3_MODULE ──→ D2.A (Anode) ──▶|── D2.K (Cathode) ──→ 3V3_BUCK net
```

| D2 Pin | Connect To | Net Label |
|--------|-----------|-----------|
| **A (Anode)** | 3V3_MODULE (J1.Pin1 / D1.Cathode) | **3V3_MODULE** |
| **K (Cathode)** | 3V3_BUCK (L1 output / C2 / U3.VCC / U2.VDD) | **3V3_BUCK** |

**Both diodes together:**

```
                    D1 (SS14)
  3V3_BUCK ────▶|─────────── 3V3_MODULE (J1.Pin1 = module 3V3)
                              │
                    D2 (SS14) │
  3V3_BUCK ────|◀─────────────┘
```

**Why 2 diodes:**
- Battery ON, no USB → D1 conducts → module gets power (3.0V after 0.3V drop)
- USB ON, no battery → Module 3V3 flows back through D2 → powers RTC + Audio
- D1 blocks USB power from reaching buck output
- D2 blocks battery power from back-feeding module

---

### E. RTC (U3 — DS3231SN) — C9866

**SO-16W package. Precision real-time clock. Many pins are NC.**

In EasyEDA, use PIN NAMES shown on the symbol.

**DS3231SN active pins only:**

| Pin Name | Connect To | Net Label |
|----------|-----------|-----------|
| **VCC** | 3V3_BUCK net | **3V3_BUCK** |
| **GND** | Ground | **GND** |
| **VBAT** | BT1 positive (+) terminal | — (direct wire) |
| **SDA** | I2C_SDA net (R2 + J1.Pin3) | **I2C_SDA** |
| **SCL** | I2C_SCL net (R3 + J1.Pin2) | **I2C_SCL** |
| **INT/SQW** | RTC_INT net (J1.Pin4) — optional | **RTC_INT** |
| **RST** | Leave NC or tie to VCC (3V3_BUCK) | — |
| **32KHz** | Leave NC | — |
| **All other NC pins** | Leave unconnected | — |

**Wiring diagram:**

```
                        DS3231SN (U3)
                     ┌──────────────────┐
                     │                  │
   NC ───────────────┤ 32KHz      SCL  ├────── I2C_SCL net ──→ R3 + J1.Pin2
                     │                  │
   3V3_BUCK ─────────┤ VCC        SDA  ├────── I2C_SDA net ──→ R2 + J1.Pin3
                     │                  │
   RTC_INT net ──────┤ INT/SQW   VBAT  ├────── BT1 (+) direct wire
   (to J1.Pin4)      │                  │
   NC ───────────────┤ RST        GND  ├────── GND
                     │                  │
                     │   (NC pins ×8)   │
                     └──────────────────┘
```

**I2C pull-up resistors (R2, R3):**

```
  3V3_BUCK ──→ R2 (4.7K) ──→ I2C_SDA net (U3.SDA + J1.Pin3)
  3V3_BUCK ──→ R3 (4.7K) ──→ I2C_SCL net (U3.SCL + J1.Pin2)
```

```
COMPONENT        PIN A CONNECTS TO          PIN B CONNECTS TO
─────────        ──────────────────          ──────────────────
R2  (4.7K)  :    Pin A → 3V3_BUCK net       Pin B → I2C_SDA net
R3  (4.7K)  :    Pin A → 3V3_BUCK net       Pin B → I2C_SCL net
```

**Battery holder (BT1) — C5365932:**

```
COMPONENT        PIN CONNECTS TO             NET LABEL
─────────        ──────────────              ─────────
BT1 (+)     :    U3.VBAT                     — (direct wire)
BT1 (-)     :    GND                         GND
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

### F. AUDIO AMP (U2 — MAX98357AETE+T) — C910544

**QFN-16 package. I2S audio amplifier with built-in DAC.**

In EasyEDA, use PIN NAMES shown on the symbol.

**MAX98357A pin connections:**

| Pin Name | Connect To | Net Label |
|----------|-----------|-----------|
| **VDD** | 3V3_BUCK net + C3 + C4 | **3V3_BUCK** |
| **GND** | Ground (multiple GND pins — connect all) | **GND** |
| **BCLK** | I2S_BCLK net (J1.Pin8) | **I2S_BCLK** |
| **LRCLK** | I2S_LRCK net (J1.Pin15) | **I2S_LRCK** |
| **DIN** | I2S_DIN net (J1.Pin14) | **I2S_DIN** |
| **SD_MODE** | AMP_EN net (J1.Pin16) | **AMP_EN** |
| **OUTP** | SPKR_P net → J3.Pin1 (Speaker +) | **SPKR_P** |
| **OUTN** | SPKR_N net → J3.Pin2 (Speaker -) | **SPKR_N** |
| **GAIN** | Leave floating (default 9dB) or connect to GND | — |

**Wiring diagram:**

```
                        MAX98357A (U2)
                     ┌──────────────────────┐
                     │                      │
   3V3_BUCK ─────────┤ VDD          OUTP   ├────── SPKR_P net → J3.Pin1 (Speaker +)
                     │                      │
   GND ──────────────┤ GND          OUTN   ├────── SPKR_N net → J3.Pin2 (Speaker -)
                     │                      │
   AMP_EN net ───────┤ SD_MODE      GAIN   ├────── NC (float) or GND
   (from J1.Pin16)   │                      │
                     │                      │
   I2S_BCLK net ────┤ BCLK                 │
   (from J1.Pin8)   │                      │
                     │                      │
   I2S_LRCK net ────┤ LRCLK                │
   (from J1.Pin15)  │                      │
                     │                      │
   I2S_DIN net ─────┤ DIN                  │
   (from J1.Pin14)  │                      │
                     └──────────────────────┘
```

**Decoupling capacitors for U2 (place close to VDD pin!):**

```
COMPONENT        PIN A CONNECTS TO          PIN B CONNECTS TO
─────────        ──────────────────          ──────────────────
C3  (10µF)  :    Pin A → 3V3_BUCK net       Pin B → GND
C4  (100nF) :    Pin A → 3V3_BUCK net       Pin B → GND
```

```
  3V3_BUCK ──┬── C3 (10µF) ── GND
             │
             ├── C4 (100nF) ── GND
             │
             └── U2.VDD
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

### G. TACTILE BUTTON (SW2) — C2939240

**HOOYA TS-1187F-015E — 4-pin SPST tactile switch.**

Internal: Pin1+Pin2 shorted (one side), Pin3+Pin4 shorted (other side).
Press = both sides connect. EasyEDA symbol shows 2 pins.

```
        ┌──────────┐
 Pin 1 ─┤          ├─ Pin 2     (Pin 1 & Pin 2 always connected)
        │  BUTTON  │
 Pin 3 ─┤          ├─ Pin 4     (Pin 3 & Pin 4 always connected)
        └──────────┘

        Press: Pin1/2 connects to Pin3/4
```

**Button circuit with pull-up:**

```
  3V3_BUCK ──→ R1 (10K) ──┬──→ BTN_WAKE net (to J1.Pin7 = GPIO8)
                           │
                      SW2 Pin 1
                           │
                      SW2 Pin 3
                           │
                          GND
```

**Connection table:**

| Component | Pin A Connects To | Pin B Connects To |
|-----------|-------------------|-------------------|
| **R1 (10K)** | 3V3_BUCK net | BTN_WAKE net (junction with SW2) |
| **SW2 Pin 1** (or Pin 2) | BTN_WAKE net (junction with R1) | — |
| **SW2 Pin 3** (or Pin 4) | GND | — |

**How it works:**
- Button NOT pressed → R1 pulls BTN_WAKE HIGH (3.3V) → GPIO8 = 1
- Button PRESSED → SW2 shorts to GND → BTN_WAKE LOW → GPIO8 = 0 (wake trigger)

### Button Behavior Map

| Context | Press | Action |
|---------|-------|--------|
| Deep sleep | Any press | Wake device, turn on display |
| Adhan playing | Short press | Stop/mute adhan |
| Normal (screen on) | Short press | Toggle mute |
| Normal (screen on) | Long press (3s) | Open settings / WiFi reconnect |
| Screen off (light sleep) | Any press | Wake display |

---

### H. SPEAKER CONNECTOR (J3) — C8269

**2-pin screw terminal. Speaker wires connect here.**

```
         J3 (2-pin Screw Terminal)
        ┌──────────────────────┐
        │                      │
        │  Pin 1       Pin 2   │
        │ (Speaker+)  (Speaker-)│
        └───┬──────────┬───────┘
            │          │
            │          └──────────→ SPKR_N net (U2.OUTN)
            │
            └─────────────────────→ SPKR_P net (U2.OUTP)
```

| J3 Pin | Connect To | Net Label |
|--------|-----------|-----------|
| **Pin 1** | U2.OUTP | **SPKR_P** |
| **Pin 2** | U2.OUTN | **SPKR_N** |

---

### I. MODULE HEADER (J1 — 2x10 Pin) — C5383109

**This is the carrier board connector that plugs into the WT32S3-28S PLUS module.**
**Use NET LABELS to connect — Press "N" in EasyEDA to place net labels.**

```
                    J1 (2x10 Pin Header)
                 ┌───────────────────────┐
                 │                       │
     3V3_MODULE ─┤ Pin 1       Pin 11   ├─ NC (5V — not used)
                 │                       │
      I2C_SCL  ─┤ Pin 2       Pin 12   ├─ NC
                 │                       │
      I2C_SDA  ─┤ Pin 3       Pin 13   ├─ NC
                 │                       │
      RTC_INT  ─┤ Pin 4       Pin 14   ├─ I2S_DIN
                 │                       │
           NC  ─┤ Pin 5       Pin 15   ├─ I2S_LRCK
                 │                       │
           NC  ─┤ Pin 6       Pin 16   ├─ AMP_EN
                 │                       │
     BTN_WAKE  ─┤ Pin 7       Pin 17   ├─ NC
                 │                       │
     I2S_BCLK  ─┤ Pin 8       Pin 18   ├─ NC
                 │                       │
           NC  ─┤ Pin 9       Pin 19   ├─ NC
                 │                       │
          GND  ─┤ Pin 10      Pin 20   ├─ GND
                 │                       │
                 └───────────────────────┘
```

**Complete J1 pin-by-pin wiring table:**

| J1 Pin | Module Pin | GPIO | Net Label | Connects To | Wire in EasyEDA |
|--------|-----------|------|-----------|-------------|-----------------|
| **1** | 3V3 | — | **3V3_MODULE** | D1.K + D2.A | Place net label "3V3_MODULE" |
| **2** | GPIO7 | 7 | **I2C_SCL** | U3.SCL + R3 | Place net label "I2C_SCL" |
| **3** | GPIO15 | 15 | **I2C_SDA** | U3.SDA + R2 | Place net label "I2C_SDA" |
| **4** | GPIO16 | 16 | **RTC_INT** | U3.INT/SQW (optional) | Place net label "RTC_INT" |
| **5** | GPIO17 | 17 | — | NC | Leave unconnected |
| **6** | GPIO18 | 18 | — | NC | Leave unconnected |
| **7** | GPIO8 | 8 | **BTN_WAKE** | R1 + SW2 | Place net label "BTN_WAKE" |
| **8** | GPIO3 | 3 | **I2S_BCLK** | U2.BCLK | Place net label "I2S_BCLK" |
| **9** | GPIO46 | 46 | — | NC | Leave unconnected |
| **10** | GND | — | **GND** | Ground | Place net label "GND" |
| **11** | 5V | — | — | NC (not used) | Leave unconnected |
| **12** | GPIO6 | 6 | — | NC | Leave unconnected |
| **13** | GPIO5 | 5 | — | NC | Leave unconnected |
| **14** | GPIO4 | 4 | **I2S_DIN** | U2.DIN | Place net label "I2S_DIN" |
| **15** | GPIO2 | 2 | **I2S_LRCK** | U2.LRCLK | Place net label "I2S_LRCK" |
| **16** | GPIO1 | 1 | **AMP_EN** | U2.SD_MODE | Place net label "AMP_EN" |
| **17** | GPIO42 | 42 | — | NC | Leave unconnected |
| **18** | GPIO41 | 41 | — | NC | Leave unconnected |
| **19** | GPIO40 | 40 | — | NC | Leave unconnected |
| **20** | GND | — | **GND** | Ground | Place net label "GND" |

**How net labels work in EasyEDA:**
1. Press **"N"** → type net label name (e.g., "I2C_SCL")
2. Place it on J1 Pin 2 wire
3. Place same "I2C_SCL" label on U3.SCL pin wire
4. Place same "I2C_SCL" label on R3 pin wire
5. EasyEDA automatically connects all 3 — no physical wire needed between them!

---

## Complete Net Label Reference

Press **"N"** in EasyEDA to add these net labels. Every net label below must appear on
ALL the pins/wires listed in the "Appears On" column.

| Net Label | Voltage | Appears On (place label on ALL of these) |
|-----------|---------|------------------------------------------|
| **GND** | 0V | J2.Pin2, U1.Pin2, C1(-), C2(-), R5 one side, D3.Anode, U3.GND, BT1(-), U2.GND, C3(-), C4(-), SW2.Pin3, J1.Pin10, J1.Pin20 |
| **VIN** | ~6V | SW1.Pin2, U1.Pin5 (IN), U1.Pin4 (EN), C1(+) |
| **3V3_BUCK** | 3.3V | L1 output side, C2(+), R4 one side, D1.Anode, D2.Cathode, U3.VCC, U2.VDD, C3(+), C4(+), R1 one side, R2 one side, R3 one side |
| **3V3_MODULE** | ~3.0V | D1.Cathode, D2.Anode, J1.Pin1 |
| **I2C_SDA** | signal | U3.SDA, R2 other side, J1.Pin3 |
| **I2C_SCL** | signal | U3.SCL, R3 other side, J1.Pin2 |
| **I2S_BCLK** | signal | U2.BCLK, J1.Pin8 |
| **I2S_LRCK** | signal | U2.LRCLK, J1.Pin15 |
| **I2S_DIN** | signal | U2.DIN, J1.Pin14 |
| **AMP_EN** | signal | U2.SD_MODE, J1.Pin16 |
| **BTN_WAKE** | signal | R1 other side, SW2.Pin1, J1.Pin7 |
| **RTC_INT** | signal | U3.INT/SQW, J1.Pin4 |
| **SPKR_P** | audio | U2.OUTP, J3.Pin1 |
| **SPKR_N** | audio | U2.OUTN, J3.Pin2 |

---

## 65-Connection Checklist

Go through this list. Each row = one wire or net label placement.
Check off each one as you complete it in EasyEDA.

### Power Path Wires

| # | From | To | Method |
|---|------|-----|--------|
| 1 | J2.Pin1 | SW1.Pin1 | Direct wire |
| 2 | J2.Pin2 | GND | Net label "GND" |
| 3 | SW1.Pin2 | VIN net | Net label "VIN" |
| 4 | SW1.Pin3 | — | Leave NC |
| 5 | U1.Pin5 (IN) | VIN net | Net label "VIN" |
| 6 | U1.Pin4 (EN) | VIN net | Net label "VIN" |
| 7 | U1.Pin2 (GND) | GND | Net label "GND" |
| 8 | C1 (+) | VIN net | Net label "VIN" |
| 9 | C1 (-) | GND | Net label "GND" |
| 10 | U1.Pin1 (BST) | C5 Pin A | Direct wire |
| 11 | U1.Pin6 (SW) | C5 Pin B | Direct wire |
| 12 | U1.Pin6 (SW) | L1 Pin A | Direct wire (same node as #11) |
| 13 | D3.Cathode (K) | SW node (same as #11, #12) | Direct wire to SW junction |
| 14 | D3.Anode (A) | GND | Net label "GND" |
| 15 | L1 Pin B | 3V3_BUCK net | Net label "3V3_BUCK" |
| 16 | C2 (+) | 3V3_BUCK net | Net label "3V3_BUCK" |
| 17 | C2 (-) | GND | Net label "GND" |
| 18 | R4 Pin A | 3V3_BUCK net | Net label "3V3_BUCK" |
| 19 | R4 Pin B | R5 Pin A + U1.Pin3 (FB) | Direct wire (junction) |
| 20 | R5 Pin B | GND | Net label "GND" |

### Diode Wires (Power Path Isolation)

| # | From | To | Method |
|---|------|-----|--------|
| 21 | D1.Anode (A) | 3V3_BUCK net | Net label "3V3_BUCK" |
| 22 | D1.Cathode (K) | 3V3_MODULE net | Net label "3V3_MODULE" |
| 23 | D2.Anode (A) | 3V3_MODULE net | Net label "3V3_MODULE" |
| 24 | D2.Cathode (K) | 3V3_BUCK net | Net label "3V3_BUCK" |

### RTC Wires

| # | From | To | Method |
|---|------|-----|--------|
| 25 | U3.VCC | 3V3_BUCK net | Net label "3V3_BUCK" |
| 26 | U3.GND | GND | Net label "GND" |
| 27 | U3.VBAT | BT1 (+) | Direct wire |
| 28 | BT1 (-) | GND | Net label "GND" |
| 29 | U3.SDA | I2C_SDA net | Net label "I2C_SDA" |
| 30 | U3.SCL | I2C_SCL net | Net label "I2C_SCL" |
| 31 | U3.INT/SQW | RTC_INT net | Net label "RTC_INT" |
| 32 | R2 Pin A | 3V3_BUCK net | Net label "3V3_BUCK" |
| 33 | R2 Pin B | I2C_SDA net | Net label "I2C_SDA" |
| 34 | R3 Pin A | 3V3_BUCK net | Net label "3V3_BUCK" |
| 35 | R3 Pin B | I2C_SCL net | Net label "I2C_SCL" |

### Audio Wires

| # | From | To | Method |
|---|------|-----|--------|
| 36 | U2.VDD | 3V3_BUCK net | Net label "3V3_BUCK" |
| 37 | U2.GND | GND | Net label "GND" |
| 38 | U2.BCLK | I2S_BCLK net | Net label "I2S_BCLK" |
| 39 | U2.LRCLK | I2S_LRCK net | Net label "I2S_LRCK" |
| 40 | U2.DIN | I2S_DIN net | Net label "I2S_DIN" |
| 41 | U2.SD_MODE | AMP_EN net | Net label "AMP_EN" |
| 42 | U2.OUTP | SPKR_P net | Net label "SPKR_P" |
| 43 | U2.OUTN | SPKR_N net | Net label "SPKR_N" |
| 44 | U2.GAIN | GND or leave NC | Net label "GND" or NC |
| 45 | C3 (+) | 3V3_BUCK net | Net label "3V3_BUCK" |
| 46 | C3 (-) | GND | Net label "GND" |
| 47 | C4 (+) | 3V3_BUCK net | Net label "3V3_BUCK" |
| 48 | C4 (-) | GND | Net label "GND" |

### Button Wires

| # | From | To | Method |
|---|------|-----|--------|
| 49 | R1 Pin A | 3V3_BUCK net | Net label "3V3_BUCK" |
| 50 | R1 Pin B | BTN_WAKE net | Net label "BTN_WAKE" |
| 51 | SW2 Pin 1 | BTN_WAKE net | Net label "BTN_WAKE" |
| 52 | SW2 Pin 3 | GND | Net label "GND" |

### Speaker Connector Wires

| # | From | To | Method |
|---|------|-----|--------|
| 53 | J3.Pin1 | SPKR_P net | Net label "SPKR_P" |
| 54 | J3.Pin2 | SPKR_N net | Net label "SPKR_N" |

### Module Header Net Labels (J1)

| # | J1 Pin | Net Label to Place |
|---|--------|-------------------|
| 55 | J1.Pin1 | 3V3_MODULE |
| 56 | J1.Pin2 | I2C_SCL |
| 57 | J1.Pin3 | I2C_SDA |
| 58 | J1.Pin4 | RTC_INT |
| 59 | J1.Pin7 | BTN_WAKE |
| 60 | J1.Pin8 | I2S_BCLK |
| 61 | J1.Pin10 | GND |
| 62 | J1.Pin14 | I2S_DIN |
| 63 | J1.Pin15 | I2S_LRCK |
| 64 | J1.Pin16 | AMP_EN |
| 65 | J1.Pin20 | GND |

**Total: 65 connections. When all 65 are done, your schematic is complete.**

---

## Complete Power Flow Diagram

```
  4×AA Battery (6V)
       │
    J2.Pin1 (BATT+)
       │
    SW1.Pin1 ←──── Slide switch
       │
    SW1.Pin2 (middle/common)
       │
      VIN (6V)──── C1 (22µF) ──── GND
       │
  ┌────┴────┐
  │ U1 Pin5 │ (IN)
  │ U1 Pin4 │ (EN = tied to IN)
  │ MP2359  │
  │ U1 Pin6 │ (SW) ── C5 ── U1.Pin1 (BST)
  └────┬────┘   │
       │    GND─▶├─┘ D3 (SS14 catch diode)
    L1 (10µH)
       │
    3V3_BUCK (3.3V) ──── C2 (22µF) ──── GND
       │
       ├──── R4 (49.9K) ──┬── U1.Pin3 (FB)
       │                │
       │             R5 (16.2K)
       │                │
       │               GND
       │
       ├──── D1.A ──▶|── D1.K ──── J1.Pin1 (WT32S3 3V3)
       │                                 │
       ├────────────── D2.K ──|◀── D2.A ┘
       │
       ├──── U3.VCC (DS3231 RTC)
       │
       ├──── U2.VDD (MAX98357A Audio) + C3 (10µF) + C4 (100nF)
       │
       ├──── R1 (10K) ──→ BTN_WAKE ──→ J1.Pin7 (GPIO8)
       │                      │
       │                   SW2.Pin1──SW2.Pin3──GND
       │
       ├──── R2 (4.7K) ──→ I2C_SDA ──→ U3.SDA + J1.Pin3 (GPIO15)
       │
       └──── R3 (4.7K) ──→ I2C_SCL ──→ U3.SCL + J1.Pin2 (GPIO7)


  U2.OUTP ──→ SPKR_P ──→ J3.Pin1 (Speaker +)
  U2.OUTN ──→ SPKR_N ──→ J3.Pin2 (Speaker -)

  U3.VBAT ──→ BT1(+)     BT1(-) ──→ GND

  J2.Pin2 (BATT-) ──→ GND
  J1.Pin10 ──→ GND
  J1.Pin20 ──→ GND
```

---

## Power Architecture & USB Isolation

### Dual-Path Power with SS14 Diodes

The system runs from 4×AA batteries or USB-C. Two SS14 Schottky diodes provide bidirectional isolation: D1 isolates battery power from the module, D2 allows USB power (via module 3V3) to feed RTC and Audio when batteries are off.

```
    4×AA BATTERIES (6V)                    MODULE USB-C (5V)
           │                                     │
        ┌──┴──┐                                  │
        │ ON  │  SPDT Slide Switch               │
        │ OFF │  (SS12D00G6)                     │
        └──┬──┘                                  │
           ▼                                     ▼
    ┌─────────────┐                      ┌──────────────────┐
    │   MP2359    │                      │ Module Internal  │
    │  Buck Conv  │                      │ LDO              │
    │  6V → 3.3V  │                      │  5V → 3.3V       │
    └──────┬──────┘                      └────────┬─────────┘
           │                                      │
           │ 3.3V                                 │ 3.3V
           │                                      │
           │  SS14 (D1)                           │
           ├───▶├──────────┐                      │
           │               │                      │
           │          3.0V │ (after drop)         │
           │               │                      │
           │          ┌────┴──────────────────────┤
           │          │    WT32S3-28S MODULE      │
           │          └───▶│ 3V3 Pin (I/O)   │◀───┘
           │               └─────────────────┘
           │
           │  SS14 (D2) reverse path
           │  ────|◀── Module 3V3 → powers peripherals via USB
           │
    ┌──────┴─────────────────────────┐
    │                                │
    ▼                                ▼
┌─────────────┐              ┌───────────────┐
│  DS3231SN   │              │  MAX98357A    │
│    RTC      │              │    Audio      │
└─────────────┘              └───────────────┘
```

### Operating Scenarios

| Scenario | Battery | USB-C | Switch | What Happens |
|----------|---------|-------|--------|--------------|
| **Normal Operation** | ✅ Inserted | ❌ Disconnected | ON | MP2359 → 3V3_BUCK powers RTC+Audio directly. D1 conducts → module gets ~3.0V (after 0.3V drop) |
| **USB Powered** | ❌ Removed | ✅ Connected | — | Module LDO → 3.3V. D2 conducts → 3V3_BUCK gets ~3.0V → powers RTC+Audio |
| **Both Connected** | ✅ Inserted | ✅ Connected | ON | See detailed explanation below |
| **Sleep Mode** | ✅ Inserted | ❌ Disconnected | ON | MP2359 in PFM mode, ~40µA total |
| **Power OFF** | ✅ Inserted | ❌ Disconnected | OFF | 0µA draw — battery lasts years in storage |

**Both Connected — Detailed Explanation:**

Both 3V3_BUCK (MP2359) and J1.Pin1 (module LDO) are at 3.3V:

| Diode | Anode | Cathode | Voltage Diff | State |
|-------|-------|---------|-------------|-------|
| D1 (3V3_BUCK → J1.Pin1) | 3.3V | 3.3V | ~0V (below 0.3V threshold) | **OFF** |
| D2 (J1.Pin1 → 3V3_BUCK) | 3.3V | 3.3V | ~0V (below 0.3V threshold) | **OFF** |

Both diodes are OFF — no current flows through either. Each path powers its own devices independently:

```
Battery → MP2359 → 3V3_BUCK (3.3V) → RTC, Audio, Pull-ups  (direct, no diode)
USB 5V  → Module LDO → J1.Pin1 (3.3V) → ESP32-S3, Display  (internal to module)
                     D1 OFF ↕ D2 OFF
              (no cross-feeding, no conflict)
```

### Battery Voltage Range

| Battery State | Voltage |
|---------------|---------|
| Fresh Alkaline | 6.0V – 6.4V |
| Normal Use | 4.8V – 6.0V |
| Depleted | 4.0V – 4.8V |
| MP2359 Min Input | 4.5V |

---

## Battery Life Calculation

### Component Current Draw (from datasheets)

| Component | Active | Sleep/Shutdown | Source |
|-----------|--------|----------------|--------|
| ESP32-S3R8 | 40–50mA | 40µA (PSRAM) | Espressif Datasheet |
| WT32S3 Display | 70mA (backlight) | 0 (IO45 LOW) | ST7789V Datasheet |
| MAX98357A @ 60% vol | 85mA @ 3.3V | 0.6µA (SD=LOW) | MAX98357A Datasheet |
| DS3231 RTC | 0.2mA | 0.2mA | DS3231 Datasheet |

> **Note:** MAX98357A @ 3.3V into 4Ω BTL: max output = V²/(2×R) = 3.3²/(2×4) = **1.36W**. At 60% volume: ~0.25W output, ~85mA supply. At max volume: ~500mA supply.

> **Max Load Warning:** At full volume + WiFi + display ON, total 3V3_BUCK load can reach ~871mA (of 1A MP2359 limit).

### Daily Energy Consumption

| State | Current (3.3V) | Duration | Energy |
|-------|----------------|----------|--------|
| Adhan (ESP32 + audio, screen OFF) | 135mA | 6 min (0.1h) | 13.5 mAh |
| Display ON (touch wake) | 120mA | 1.75 min (0.029h) | 3.5 mAh |
| Deep Sleep (ESP32 + RTC) | 40µA | 23.87 h | 1.0 mAh |
| **Subtotal** | | | **18 mAh** |
| Buck converter loss (90% eff.) | ×1.11 | | +2 mAh |
| **Total per day** | | | **~20 mAh** |

### Battery Life Estimate

```
4×AA Batteries (2400 mAh) ÷ 20 mAh/day = 120 days ≈ 4 months
```

| Usage Pattern | Battery Life |
|---------------|--------------|
| Optimized (6 min ezan, 2 min screen) | **~4 months** |
| Moderate (+ 10 min screen/day) | ~2.5 months |
| Heavy (+ 30 min screen/day) | ~1 month |

### Power Optimization Requirements

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
│     ├─ Place SS14 diodes near module connector                         │
│     ├─ Short traces from diode cathodes to 3.3V rail                   │
│     ├─ D1: Battery path isolation (after buck converter)               │
│     └─ D2: USB path isolation (from module 3V3)                        │
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
│                                                                        │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │             WT32S3-28S PLUS MODULE                              │   │
│  │               (Mounted on headers)                              │   │
│  │           77 × 47mm with 2.8" display                           │   │
│  └───────────────────────────┬─────────────────────────────────────┘   │
│                              │                                         │
│  ┌────────────┐  ┌────────────┐  ┌─────────────┐  ┌──────────────┐    │
│  │   MP2359   │  │  D1 + D2   │  │  DS3231SN   │  │  MAX98357A   │    │
│  │  Buck + D3 │  │  SS14 ×2   │  │  + CR1220   │  │  + C3 + C4   │    │
│  │ +C1,C2,C5 │  │            │  │  + R2 + R3  │  │              │    │
│  │ +L1,R4,R5 │  │            │  │             │  │              │    │
│  └────────────┘  └────────────┘  └─────────────┘  └──────────────┘    │
│                                                                        │
│  ┌────────────┐  ┌─────────┐      ┌─────────┐     ┌────────────────┐  │
│  │ SW1 Slide  │  │ SW2+R1  │      │  J1     │     │  J3 Speaker   │  │
│  │  Switch    │  │ Button  │      │ Header  │     │  Connector    │  │
│  └────────────┘  └─────────┘      │ 2×10    │     └────────────────┘  │
│                                    └─────────┘                         │
│  ┌────────────────┐                                                    │
│  │ J2 Battery     │                                                    │
│  │ Connector      │                                                    │
│  └────────────────┘                                                    │
│                                                                        │
│  Board size: 65mm × 50mm                                               │
└────────────────────────────────────────────────────────────────────────┘
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

### Schematic Net List (for EasyEDA/KiCad Import)

```
NET LIST - Carrier Base Board for WT32S3-28S PLUS (4×AA Version with USB Isolation)
====================================================================================

Power Nets:
-----------
NET: VIN (4.5V-6.4V from batteries via slide switch)
  - SW1.Pin2 (slide switch common)
  - U1.Pin5 (MP2359 IN)
  - U1.Pin4 (MP2359 EN — tied to IN)
  - C1 (+) input capacitor

NET: 3V3_BUCK (MP2359 output + USB reverse feed)
  - L1 output side (via MP2359 LX)
  - C2 (+) output cap
  - R4 one side (49.9K feedback high)
  - D1 Anode (battery → module isolation)
  - D2 Cathode (module → peripherals feed)
  - U3.VCC (DS3231 RTC)
  - U2.VDD (MAX98357A Audio)
  - C3 (+) audio decoupling
  - C4 (+) audio decoupling
  - R1 one side (10K button pull-up)
  - R2 one side (4.7K I2C SDA pull-up)
  - R3 one side (4.7K I2C SCL pull-up)

NET: 3V3_MODULE (module's 3V3 pin)
  - D1 Cathode (receives battery power)
  - D2 Anode (outputs USB power to peripherals)
  - J1.Pin1 (WT32S3-28S 3V3 pin)

NET: GND
  - J2.Pin2 (battery negative)
  - U1.Pin2 (MP2359 GND)
  - D3 Anode (SS14 freewheeling diode)
  - C1 (-), C2 (-), C3 (-), C4 (-)
  - R5 one side (16.2K feedback low)
  - U3.GND (DS3231)
  - BT1 (-) (CR1220 holder)
  - U2.GND (MAX98357A — all GND pins)
  - SW2.Pin3 (tactile button)
  - J1.Pin10, J1.Pin20

Signal Nets:
------------
NET: I2C_SDA
  - J1.Pin3 (GPIO15)
  - U3.SDA (DS3231)
  - R2 other side (4.7K pull-up)

NET: I2C_SCL
  - J1.Pin2 (GPIO7)
  - U3.SCL (DS3231)
  - R3 other side (4.7K pull-up)

NET: I2S_BCLK
  - J1.Pin8 (GPIO3)
  - U2.BCLK (MAX98357A)

NET: I2S_LRCK
  - J1.Pin15 (GPIO2)
  - U2.LRCLK (MAX98357A)

NET: I2S_DIN
  - J1.Pin14 (GPIO4)
  - U2.DIN (MAX98357A)

NET: AMP_EN
  - J1.Pin16 (GPIO1)
  - U2.SD_MODE (MAX98357A)

NET: RTC_INT (optional)
  - J1.Pin4 (GPIO16)
  - U3.INT/SQW (DS3231)

NET: BTN_WAKE
  - J1.Pin7 (GPIO8)
  - R1 other side (10K pull-up)
  - SW2.Pin1 (tactile switch → pressed = GND)

NET: SPKR_P
  - U2.OUTP (MAX98357A)
  - J3.Pin1 (Speaker +)

NET: SPKR_N
  - U2.OUTN (MAX98357A)
  - J3.Pin2 (Speaker -)

Internal Buck Converter Nets:
-----------------------------
NET: SW (switching node)
  - U1.Pin6 (MP2359 SW)
  - C5 Pin B (bootstrap cap)
  - L1 Pin A (inductor input)
  - D3 Cathode (SS14 freewheeling diode)

NET: BST (bootstrap)
  - U1.Pin1 (MP2359 BST)
  - C5 Pin A (bootstrap cap)

NET: FB (feedback)
  - U1.Pin3 (MP2359 FB)
  - R4 Pin B (49.9K high side)
  - R5 Pin A (16.2K low side)
```

---

## EasyEDA: ERC, PCB Conversion & Ordering

### Power Flags

EasyEDA may show warnings about undriven power nets. To fix:
1. Press **"L"** → Search **"PWR_FLAG"**
2. Place one on the **VIN** net
3. Place one on the **GND** net
4. Place one on the **3V3_BUCK** net

### Run ERC (Electrical Rules Check)

1. Click **"Design" → "Check ERC"**
2. Fix any errors (usually missing power flags or unconnected pins)
3. Unconnected NC pins are OK — ignore those warnings

### Convert to PCB

1. Click **"Design" → "Convert to PCB"**
2. Set board outline: **65mm × 50mm**
3. Arrange components per the placement diagram above
4. Route traces:
   - Power traces (VIN, 3V3_BUCK, GND): **0.5mm width**
   - Signal traces (I2C, I2S, BTN): **0.25mm width**
5. Add ground pour on **bottom layer**

### Order from JLCPCB

1. Click **"Fabrication" → "One-click Order PCB/SMT"**
2. BOM and CPL auto-generated
3. Enable **SMT Assembly**
4. Review parts matching
5. Checkout

---

## Notes & Warnings

1. **EN pin MUST connect to VIN** (not GND!) — otherwise buck converter is disabled
2. **Bootstrap cap C5** goes between BST (Pin 1) and SW (Pin 6) — NOT to GND
3. **D3 (catch diode) is REQUIRED** — Anode to GND, Cathode to LX. Without it the buck converter will not function!
4. **D1 and D2 direction matters** — check Anode (A) vs Cathode (K) carefully
5. Place **decoupling caps C3, C4 close to U2.VDD** pin on PCB
6. Place **C1 close to U1.IN** pin, **C2 close to L1 output** on PCB
7. **R4/R5 feedback divider** connects to U1.FB — place close to Pin 3 on PCB
8. **All LCSC part numbers** verified for JLCPCB stock
9. **Bottom layer** = solid GND pour (copper fill)
10. J1 Pin 11 (5V) is **not used** in this design — leave NC
11. The DS3231 RST pin can be left NC or tied to 3V3_BUCK for safety
12. MAX98357A GAIN left floating = default 9dB gain

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-14 | Initial design (WT32S3-28S PLUS module-based) |
| 1.1 | 2026-02-07 | Changed from Li-ion to 4×AA batteries |
| 1.2 | 2026-02-07 | Added detailed battery life calculation with datasheet values (~4 months) |
| 1.3 | 2026-02-07 | Simplified — removed carrier USB-C (uses module's USB-C for programming) |
| 1.4 | 2026-02-07 | Added SS14 USB isolation — prevents reverse current when USB+battery connected |
| 1.5 | 2026-02-09 | Added SPDT power switch (ON/OFF) and physical button (GPIO8) for wake/mute/settings |
| 1.6 | 2026-02-09 | Added 2nd SS14 diode (D2) so USB-C can fully power device (RTC + Audio) without batteries |
| 1.7 | 2026-02-14 | Merged SCHEMATIC.md + EASYEDA_WIRING_GUIDE.md into single complete document |
| 1.8 | 2026-02-14 | Added D3 (SS14 freewheeling diode) — CRITICAL missing component for MP2359 buck converter |
| 1.9 | 2026-02-14 | Fixed MP2359 feedback resistors (R4: 18K→49.9K 1%, R5: 3.9K→16.2K 1%), corrected Vref from 0.6V to 0.81V, fixed pinout (Pin1=BST, Pin5=IN, Pin6=SW) per actual datasheet |
| 2.0 | 2026-02-14 | Upgraded L1 inductor from 0805/C1046 (~0.4A) to 1210/XRTC322512S100MBCA (2.2A Isat) — old inductor would saturate at max volume. Fixed MAX98357A power calc (4Ω not 8Ω). Added PCB layout rules from MP2359 datasheet Fig.2 |
