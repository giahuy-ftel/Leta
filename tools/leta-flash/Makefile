##############################################################################
# Leta USB DFU flasher
##############################################################################

MODULE = leta-flash

FW_PATH ?= ../../application/build_OLED_WATCH/OLED_WATCH.bin
DFU_PATH ?= build_$(MODULE)/OLED_WATCH.dfu
DFU_UTIL ?= dfu-util
USB_RULES_FILE = usb_rules/leta_usb_dfu.rules

OPTIMIZE = -g -Os
CXX = g++
OBJ_DIR = build_$(MODULE)

CXXFLAGS += -Isources

VPATH += sources

OBJ += $(OBJ_DIR)/app_dbg.o
OBJ += $(OBJ_DIR)/firmware.o
OBJ += $(OBJ_DIR)/usb_dfu.o
OBJ += $(OBJ_DIR)/leta_flash.o

CXXFLAGS += $(OPTIMIZE) \
	-std=c++11 \
	-Wall \
	-Winline \
	-Wno-unused-result \
	-pipe

LDFLAGS += -Wl,-Map=$(OBJ_DIR)/$(MODULE).map

.PHONY: help all pack flash install usb-rules clean

all: create $(OBJ_DIR)/$(MODULE)

help:
	@echo "[make] build host leta-flash USB DFU helper"
	@echo "[make pack FW_PATH=../../application/build_OLED_WATCH/OLED_WATCH.bin] build DFU container"
	@echo "[make flash FW_PATH=../../application/build_OLED_WATCH/OLED_WATCH.bin] build DFU container and flash by dfu-util"
	@echo "[make install] install leta-flash and USB DFU rules"

create:
	@echo mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)

$(OBJ): | create

$(OBJ_DIR)/%.o: %.cpp | create
	@echo CXX $<
	@$(CXX) -c -o $@ $< $(CXXFLAGS)

$(OBJ_DIR)/$(MODULE): $(OBJ)
	@echo ---------- START LINK PROJECT ----------
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

pack: $(OBJ_DIR)/$(MODULE)
	@$(OBJ_DIR)/$(MODULE) pack $(FW_PATH) $(DFU_PATH)

flash: $(OBJ_DIR)/$(MODULE)
	@$(OBJ_DIR)/$(MODULE) flash $(FW_PATH) $(DFU_PATH)

install: $(OBJ_DIR)/$(MODULE) usb-rules
	@echo "Installing $(MODULE) to /usr/local/bin/..."
	@cp $(OBJ_DIR)/$(MODULE) /usr/local/bin/

usb-rules:
	@echo "Installing USB DFU rules..."
	@cp $(USB_RULES_FILE) /etc/udev/rules.d/
	@udevadm control --reload-rules
	@udevadm trigger

clean:
	@echo rm -rf $(OBJ_DIR)
	@rm -rf $(OBJ_DIR)
