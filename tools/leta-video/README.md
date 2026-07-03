# Leta Video

`leta-video` streams a selected X11 desktop region to the Leta `USB Video` page.

The watch receives a raw `128x64` monochrome OLED framebuffer over HID OUT reports:

```text
16 reports * 64 bytes = 1024 bytes
```

The byte layout is SH1106/OLED page order: 8 vertical pages by 128 columns.

## Build

```sh
make -C tools/leta-video
```

## Run

Install the udev rule once:

```sh
sudo make -C tools/leta-video install
```

Open the `USB Video` page on the watch, then run one command:

```sh
tools/leta-video/build_leta-video/leta-video
```

Drag-select a screen region. The utility converts that region to `128x64` and streams it until `Ctrl+C`.

Useful options:

```sh
tools/leta-video/build_leta-video/leta-video --invert
tools/leta-video/build_leta-video/leta-video --fps 20 --threshold 140
```

For debugging device discovery, use `tools/leta-video/build_leta-video/leta-video --list`.

This utility currently captures through X11.
