# Leta
```The name:``` Leta is summer in russia. It is the time that i started to make my own wrist watch.

<figure>
  <img
  src="assets/P1210208.JPG">
  <figcaption align=center>Oled wrist watch using Stm32f1 with mini RTOS and Monochrome GUI</figcaption>
</figure>

# Build And Flash

Leta now uses a USB DFU bootloader. The application is linked at `0x08007000`; the bootloader lives at `0x08000000`.

## Requirements

+ ARM GCC toolchain: `arm-none-eabi-gcc`
+ `dfu-util` for USB DFU flashing
+ ST-Link tools or OpenOCD for first-time bootloader flashing

The repo can also use the bundled toolchain under `tools/toolchain/` when present.

## Build

```sh
make app
make bootloader
```

Useful top-level targets:

```sh
make help
```

## First-Time Programming

Flash the bootloader with ST-Link:

```sh
make flash-bootloader-stlink
```

After the bootloader is installed, enter USB DFU by holding the top-right button, `TR` / `PA15`, while resetting or powering on the watch.

## Flash App Over USB DFU

With the watch in DFU mode:

```sh
make flash-app-dfu
```

This builds the app, packs it into the Leta DFU container, and runs `dfu-util`.

## Flash App With ST-Link

For direct app flashing without DFU:

```sh
make flash-app-stlink
```

## Memory Map

```text
0x08000000 - 0x08005FFF  bootloader
0x08006000 - 0x08006FFF  shared boot state
0x08007000 - 0x0801FFFF  application
```

Normal boot jumps straight to the app. The 1 second wait only happens after a successful DFU download so `dfu-util` can read the final status before the bootloader jumps to the app.


# Demo
<div align="center">
    <video src="https://github.com/user-attachments/assets/c9f8b56e-a367-4709-93c1-a5930ff01f5d" alt="Leta demo" />
</div>

Link youtube:

https://www.youtube.com/watch?v=F7zo0RkRJ88

# Schematic

<figure>
  <img
  src="assets/Schema.png">
</figure>

+ Microcontroller: Stm32f103xx, 128 kB ROM, 20 kB RAM. Build-int RTC.
+ IMU: MPU6050, HCM5883L.
+ Temp and humid: HTU21D.
+ Bluetooth: RF-BM-4044B4(RF-Star).
+ Display: SH1106 oled 1.3" 128x64.
+ Power: TP4056 charger IC, regulator 3.3V AP2112.

# PCB
<figure>
  <img
  src="assets/Pcb.png">
</figure>

+ 2 layers
+ USB interface
+ 1 port SWD interface
+ 4 buttons

<figure>
  <img
  src="assets/3d_view.png">
</figure>

<figure>
  <img
  src="assets/3_pcb.png">
</figure>

<figure>
  <img
  src="assets/assembled.jpg">
</figure>

# Enclosure

<figure>
  <img
  src="assets/case.gif">
</figure>

# Program layers

<figure>
  <img
  src="assets/program_layer.png">
</figure>

## AKOS
This project uses AKOS from the AK Foundation:

https://github.com/the-ak-foundation/akos

AKOS provides:
+ Preemptive scheduling
+ Round-robin scheduling
+ Thread messaging
+ Software timers

## Oled GUI
To control oled display with rich animating. I wrote the library to increase experience on UI.

The UI has some widget like:
+ Switch
+ Label text
+ Check box
+ Bitmap
+ Indicator
+ Pop-up notify
+ Slider

Animating can do these things to widgets:
+ Fading
+ Moving
+ Resizing

Check out here is more detail about my GUI:

https://github.com/snoopy3921/Monochrome-Oled-GUI

# References
These links below are materials which helped me a lot when bulding my project:

+ https://github.com/ZeekiChen/iWatch-v2.0
+ https://github.com/lvgl/lvgl
+ https://github.com/Sheep118/WouoUI-PageVersion
+ https://github.com/ak-embedded-software/ak-base-kit-stm32l151
+ https://github.com/FreeRTOS/FreeRTOS-Kernel
