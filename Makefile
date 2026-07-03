Print = @echo

APP_DIR = application
BOOTLOADER_DIR = bootloader
HOST_AK_FLESH_DIR = tools/ak-flesh
HOST_LETA_FLASH_DIR = tools/leta-flash
HOST_LETA_VIDEO_DIR = tools/leta-video

.PHONY: help all app bootloader ak-flesh leta-flash leta-video install clean clean-app clean-bootloader clean-host flash-app-dfu flash-app-uart flash-app-stlink flash-bootloader-stlink flash-stlink verify reset debug com

all: app bootloader

help:
	$(Print) "Top-level project selector"
	$(Print) "[make app] build Leta application at 0x08007000"
	$(Print) "[make bootloader] build USB DFU bootloader at 0x08000000"
	$(Print) "Bootloader default jumps to Leta; hold TR/PA15 at reset to enter USB DFU"
	$(Print) "[make flash-app-dfu] flash Leta app over USB DFU"
	$(Print) "[make flash-app-stlink] write relocated Leta app via ST-Link"
	$(Print) "[make flash-bootloader-stlink] write USB DFU bootloader via ST-Link"
	$(Print) "[make leta-flash] build USB DFU host helper"
	$(Print) "[make leta-video] build USB Video screen streamer"
	$(Print) "[sudo make install] install USB DFU host helper and udev rules"
	$(Print) "[make clean] clean firmware and host projects"

app:
	$(MAKE) -C $(APP_DIR)

bootloader:
	$(MAKE) -C $(BOOTLOADER_DIR)

ak-flesh:
	$(MAKE) -C $(HOST_AK_FLESH_DIR)

leta-flash:
	$(MAKE) -C $(HOST_LETA_FLASH_DIR)

leta-video:
	$(MAKE) -C $(HOST_LETA_VIDEO_DIR)

install:
	$(MAKE) -C $(HOST_LETA_FLASH_DIR) install
	$(MAKE) -C $(HOST_LETA_VIDEO_DIR) install

flash-app-dfu: app leta-flash
	$(MAKE) -C $(HOST_LETA_FLASH_DIR) flash FW_PATH=$(CURDIR)/$(APP_DIR)/build_OLED_WATCH/OLED_WATCH.bin

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
	@if [ -d "$(HOST_AK_FLESH_DIR)" ]; then $(MAKE) -C $(HOST_AK_FLESH_DIR) clean; fi
	$(MAKE) -C $(HOST_LETA_FLASH_DIR) clean
	$(MAKE) -C $(HOST_LETA_VIDEO_DIR) clean

verify:
	$(MAKE) -C $(APP_DIR) verify

reset:
	$(MAKE) -C $(APP_DIR) reset

debug:
	$(MAKE) -C $(APP_DIR) debug

com:
	$(MAKE) -C $(APP_DIR) com dev=$(dev)
