# AK - Flesh
[<img src="https://github.com/the-ak-foundation/ak-base-kit-stm32l151/blob/main/hardware/images/ak-foundation-logo.png" width="240"/>](https://github.com/the-ak-foundation)

`ak-flesh` is the launcher-aware firmware/app flashing tool for this project.

It is inspired by and inherited from the original [`ak-flash`](https://github.com/the-ak-foundation/ak-flash) UART firmware updater, but this fork now understands this launcher workflow, including `internal` MCU flashing and `external` W25Q app-slot flashing.

### Installation
```sh
cd ak-flesh
make
sudo make install
```

### Example
```sh
ak-flesh /dev/ttyUSB0 ak-base-kit-stm32l151-application.bin 0x08007000 internal
ak-flesh /dev/ttyUSB0 app0.bin 0x00080000 external
ak-flesh /dev/ttyUSB0 ../apps-showcase/archery 0x00080000 external
```

For external app slots, `ak-flesh` stores launcher metadata too:

- display name is taken from the firmware filename before `.bin`
- optional icon is loaded from a sibling `*.icon.bin` file
- icon format is always fixed at raw 42x42 1-bit bitmap, exactly 252 bytes

Example:

```text
archery-game.bin
archery-game.icon.bin
```

Folder mode is also supported:

```text
apps-showcase/archery/
  archery.bin
  archery.icon.bin
```

When the second argument is a folder, `ak-flesh` finds the single app `.bin` file inside and uses the matching `.icon.bin` if present.

Create the icon from a normal image:

```sh
python3 ../tools/image_to_icon.py archery-game.png archery-game.icon.bin
```

Useful conversion options:

```sh
python3 ../tools/image_to_icon.py archery-game.png archery-game.icon.bin --threshold 160
python3 ../tools/image_to_icon.py archery-game.png archery-game.icon.bin --invert
```

Then:

```sh
ak-flesh /dev/ttyUSB0 archery-game.bin 0x00080000 external
```

If the icon file is missing, the launcher falls back to text display.

### Reference
| Topic | Link |
| ------ | ------ |
| Original inherited tool | https://github.com/the-ak-foundation/ak-flash |
| Blog & Tutorial | https://epcb.vn/blogs/ak-embedded-software |
| Where to buy KIT? | https://epcb.vn/products/ak-embedded-base-kit-lap-trinh-nhung-vi-dieu-khien-mcu |
