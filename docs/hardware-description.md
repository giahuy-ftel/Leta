# Leta Hardware Description

This document summarizes the Leta watch hardware from `hardware/Sheet1.pdf` and cross-checks the signal names used by the firmware in `application/sources/platform/io_cfg.h`.

## Overview

Leta is built around an STM32F103C8T6 running from a 3.3 V rail. The board includes USB-C for power/USB data, a Li-ion battery input and charger, a controllable 3.3 V regulator, a 128x64 SH1106 OLED, four buttons, I2C motion/environment sensors, and a UART BLE module.

Main parts:

| Block | Part / interface | Notes |
| --- | --- | --- |
| MCU | STM32F103C8T6 | Cortex-M3, USB FS device, SPI1, I2C1, UART1 |
| Battery charger | TP4056 | Charges `VBAT` from USB `VBUS` |
| 3.3 V regulator | AP2112K-3.3 | Enabled by `3V3_EN` |
| Display | SH1106 OLED | SPI-like 4-wire control, 128x64 |
| IMU | MPU6050 | I2C1 |
| Magnetometer | HMC5883L | I2C1 |
| Humidity/temp sensor | HTU21D | I2C1 |
| BLE | `BL_RF` module | UART1 plus enable/reset/status lines |
| USB | USB-C receptacle | USB FS on PA11/PA12 |

## Power

The board has two main power domains:

| Net | Source | Use |
| --- | --- | --- |
| `VBUS` | USB-C connector | TP4056 charger input and USB bus presence |
| `VBAT` | Battery / charger output | Raw battery rail |
| `3.3V` | AP2112K-3.3 output | MCU, sensors, display, BLE |
| `3V3_EN` | MCU-controlled enable net | Enables the AP2112K regulator |
| `Bat_Lvl` | resistor divider from `VBAT` | Battery voltage ADC input |

The TP4056 charger has red/green status LEDs. `VBUS` feeds the charger, and the battery is connected at `VBAT`.

The AP2112K-3.3 regulator generates the `3.3V` rail from `VBAT`. Its enable pin is driven by `3V3_EN`, which is connected to MCU pin `PB0`. The power/wake button circuit also interacts with this rail so the device can be brought up from the battery.

Battery measurement uses a divider:

| Net | MCU pin | Schematic parts |
| --- | --- | --- |
| `Bat_Lvl` | `PB1` | `R20 100k`, `R19 100k`, `R3 10k`, diode path from `VBAT` |

## MCU Clocking

The MCU is an STM32F103C8T6.

| Clock | Pins | Parts |
| --- | --- | --- |
| HSE | `PD0_OSC_IN`, `PD1_OSC_OUT` | 8 MHz crystal `Y2`, 12 pF caps `C3`, `C4` |
| LSE | `PC14`, `PC15` | 32.768 kHz crystal `Y1`, 15 pF caps `C1`, `C2` |

The MCU has local decoupling on the 3.3 V rail: `C18`, `C19`, `C20`, `C21`, `C22`.

## Programming And Debug

SWD is exposed on header `P2`.

| Header signal | MCU pin |
| --- | --- |
| `SWDIO` | `PA13` |
| `SWCLK` | `PA14` |
| `3.3V` | board 3.3 V |
| `GND` | ground |

`NRST` is pulled up to 3.3 V through `R3 10k` and includes a small series resistor path near the MCU.

## USB

USB is provided through connector `J1`, a USB-C receptacle.

| USB net | MCU pin | Schematic details |
| --- | --- | --- |
| `USB_DM` | `PA11` | series resistor `R9 22R` |
| `USB_DP` | `PA12` | series resistor `R8 22R`, D+ pull-up `R7 1.5k` to 3.3 V |
| `VBUS` | charger/USB detect rail | USB-C VBUS pins |
| Shield | GND | connector shield tied to ground |

The D+ 1.5 k pull-up is present in the schematic, which is required for STM32F103 USB FS enumeration because this MCU does not provide an internal D+ pull-up.

## Buttons

There are four user buttons. Firmware names come from `io_cfg.h`.

| Schematic | Firmware name | MCU pin | Active level | Notes |
| --- | --- | --- | --- | --- |
| `Button 1` / `PA0_WKUP` | `BUTTON_RIGHT_DOWN` | `PA0` | High | Wake/power button path from `VBAT` through `D5` and `R23`; pulldown `R18 2k` |
| `Button 2` / `SW2` | `BUTTON_LEFT_DOWN` | `PB15` | Low | Used by the bootloader to enter USB DFU |
| `Button 3` / `SW3` | `BUTTON_RIGHT_UP` | `PA15` | Low | Pull-up configured in firmware |
| `Button 4` / `SW4` | `BUTTON_LEFT_UP` | `PB4` | Low | Pull-up `R11 10k` shown on the schematic |

## Display

The OLED is an SH1106 display module connected to SPI1-style signals.

| Display signal | MCU pin | Firmware net |
| --- | --- | --- |
| `OLED_DCX` | `PA4` | data/command select |
| `SPI1_SCK` | `PA5` | serial clock |
| `OLED_RST` | `PA6` | display reset |
| `SPI1_MOSI` | `PA7` | serial data |

The display block includes charge-pump and supply capacitors (`C5`, `C6`, `C7`, `C9`, `C10`) plus bias resistors (`R4`, `R6`) matching the SH1106 module requirements.

## I2C Sensors

I2C1 is shared by the environmental and motion sensors.

| I2C net | MCU pin | Pull-up |
| --- | --- | --- |
| `I2C1_SCL` | `PB6` | `R10 10k` to 3.3 V |
| `I2C1_SDA` | `PB7` | `R11 10k` to 3.3 V |

Connected I2C devices:

| Device | Schematic ref | Supply | Notes |
| --- | --- | --- | --- |
| HTU21D | `U6` | 3.3 V | Humidity/temperature sensor, `SCK` and `DATA` on I2C1 |
| HMC5883L | `U4` | 3.3 V | Magnetometer, `SCL`/`SDA` on I2C1, `DRDY` not connected |
| MPU6050 | `U9` | 3.3 V | IMU on I2C1, `AD0` tied low, `INT` routed on the symbol but not used by current firmware |

## BLE Module

The BLE module is labeled `BL_RF` in the schematic and uses UART1 plus control pins.

| BLE signal | MCU pin | Notes |
| --- | --- | --- |
| `UART1_TX` | `PA9` | MCU TX to module RX |
| `UART1_RX` | `PA10` | MCU RX from module TX |
| `BLE_EN` | `PA8` | enable control, also drives LED `D6` through `R21 1k` |
| `BLE_RST` | `PB3` | reset control |
| `BLE_STT` | `PA15` in firmware | Firmware declares this status input, but the schematic area around the BLE module should be checked before relying on it because `PA15` is also used as `SW3` |

## Firmware Pin Summary

| Function | MCU pin |
| --- | --- |
| Power enable | `PB0` |
| Battery ADC | `PB1` |
| OLED DC | `PA4` |
| OLED SCK | `PA5` |
| OLED reset | `PA6` |
| OLED MOSI | `PA7` |
| BLE enable | `PA8` |
| UART1 TX | `PA9` |
| UART1 RX | `PA10` |
| USB DM | `PA11` |
| USB DP | `PA12` |
| SWDIO | `PA13` |
| SWCLK | `PA14` |
| Button 1 / wake | `PA0` |
| Button 2 / left down | `PB15` |
| Button 3 / right up | `PA15` |
| Button 4 / left up | `PB4` |
| I2C1 SCL | `PB6` |
| I2C1 SDA | `PB7` |

## Bootloader-Relevant Notes

The USB DFU bootloader should use `PA11`/`PA12` for USB FS and may rely on the external D+ pull-up `R7`.

The bootloader must keep `3V3_EN` asserted on `PB0`; otherwise the AP2112K 3.3 V regulator can shut down and the board will lose power after the wake button is released.

When the bootloader is left in USB DFU mode, it should release `3V3_EN` after its inactivity timeout so the device can shut down instead of staying awake indefinitely.

The DFU entry button is `BUTTON_LEFT_DOWN`, which is schematic net `SW2` on `PB15`. This button is active low.

The application image is linked at `0x08007000`; the bootloader occupies the start of flash at `0x08000000`.

## Source Files

Primary hardware source:

- `hardware/Sheet1.pdf`

Firmware cross-reference:

- `application/sources/platform/io_cfg.h`
- `bootloader/sources/application/main.c`
