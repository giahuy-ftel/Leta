# Leta Flash

`leta-flash` is the USB DFU flashing helper for the Leta bootloader.

For now it builds the DFU image container expected by the bootloader:

```text
[ dfu_image_header_t ][ app.bin payload ]
```

The header contains:

- magic: `"LETA"`
- header version
- header size
- app start address: `0x08007000`
- app length
- CRC32 of the app payload

USB transfer uses `dfu-util` with VID/PID `0309:2001`.

## Build

```sh
cd tools/leta-flash
make
```

## Pack

```sh
make pack FW_PATH=../../application/build_OLED_WATCH/OLED_WATCH.bin
```

Or directly:

```sh
build_leta-flash/leta-flash pack ../../application/build_OLED_WATCH/OLED_WATCH.bin build_leta-flash/OLED_WATCH.dfu
```

## Flash

From the repo root, prefer:

```sh
make flash-app-dfu
```

Or from this directory:

```sh
make flash FW_PATH=../../application/build_OLED_WATCH/OLED_WATCH.bin
```

This prepares the `.dfu` container and runs:

```sh
dfu-util -d 0309:2001 -a 0 -D build_leta-flash/OLED_WATCH.dfu
```
