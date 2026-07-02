Print = @echo

APP_DIR = application
BOOTLOADER_DIR = bootloader
HOST_AK_FLESH_DIR = tools/ak-flesh

.PHONY: help all app bootloader ak-flesh clean clean-app clean-bootloader clean-host flash flash-dfu flash-app-uart flash-app-stlink flash-bootloader-stlink flash-stlink verify reset debug com

all: app bootloader

help:
	$(Print) "Top-level project selector"
	$(Print) "[make app] build Leta application at 0x08007000"
	$(Print) "[make bootloader] build USB DFU bootloader at 0x08000000"
	$(Print) "Bootloader default jumps to Leta; hold LD/PB15 at reset to enter USB DFU"
	$(Print) "[make flash] flash Leta app over USB DFU"
	$(Print) "[make flash-app-stlink] write relocated Leta app via ST-Link"
	$(Print) "[make flash-bootloader-stlink] write USB DFU bootloader via ST-Link"
	$(Print) "[make ak-flesh] build legacy UART host flasher"
	$(Print) "[make clean] clean firmware and host projects"

app:
	$(MAKE) -C $(APP_DIR)

bootloader:
	$(MAKE) -C $(BOOTLOADER_DIR)

ak-flesh:
	$(MAKE) -C $(HOST_AK_FLESH_DIR)

flash: flash-dfu

flash-dfu: app
	$(MAKE) -C $(BOOTLOADER_DIR) flash-dfu APP_BIN=$(CURDIR)/$(APP_DIR)/build_OLED_WATCH/OLED_WATCH.bin

flash-app-uart: app ak-flesh
	$(MAKE) -C $(HOST_AK_FLESH_DIR) flash $(if $(dev),dev=$(dev),) FW_PATH=$(CURDIR)/$(APP_DIR)/build_OLED_WATCH/OLED_WATCH.bin APP_START_ADDR_VAL=0x08007000

flash-app-stlink:
	$(MAKE) -C $(APP_DIR) flash-stlink

flash-bootloader-stlink:
	$(MAKE) -C $(BOOTLOADER_DIR) flash-stlink

flash-stlink: flash-app-stlink

clean: clean-app clean-bootloader clean-host

clean-app:
	$(MAKE) -C $(APP_DIR) clean

clean-bootloader:
	$(MAKE) -C $(BOOTLOADER_DIR) clean

clean-host:
	$(MAKE) -C $(HOST_AK_FLESH_DIR) clean

verify:
	$(MAKE) -C $(APP_DIR) verify

reset:
	$(MAKE) -C $(APP_DIR) reset

debug:
	$(MAKE) -C $(APP_DIR) debug

com:
	$(MAKE) -C $(APP_DIR) com dev=$(dev)
