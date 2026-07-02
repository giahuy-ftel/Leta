##############################################################################
# @author: GaoKong
# @date:   15/09/2017
##############################################################################

TOOL_DIR =
MODULE	 = ak-flesh

# Firmware path and device
FW_PATH  = ../application/build_OLED_WATCH/OLED_WATCH.bin
dev = /dev/ttyUSB0
# Internal app destination in the bootloader flash layout.
APP_START_ADDR_VAL = 0x08007000
FLASH_MODE = external

# USB rules
USB_RULES_FILE = usb_rules/uart_usb_conf.rules

OPTIMIZE = -g -Os
CXX	 = g++
CC	 = gcc
OBJ_DIR	 = build_$(MODULE)

CXXFLAGS += -I/usr/local/include
CXXFLAGS += -I/usr/include

CXXFLAGS += -Isources

VPATH += sources

OBJ += $(OBJ_DIR)/uart_boot.o
OBJ += $(OBJ_DIR)/firmware.o
OBJ += $(OBJ_DIR)/app_dbg.o

# CXX compiler option
CXXFLAGS	+=$(OPTIMIZE)		\
		-std=c++11		\
		-Wall			\
		-Winline		\
		-Wno-unused-result	\
		-pipe			\
		-g			\

# Library paths
LDFLAGS	+= -L/usr/local/lib
LDFLAGS	+= -L/usr/include
LDFLAGS	+= -Wl,-Map=$(OBJ_DIR)/$(MODULE).map

#Library libs
LDLIBS	+=	-lpthread		\
		-lrt			\

.PHONY: help all flash install usb-rules clean

all: create $(OBJ_DIR)/$(MODULE)

help:
	@echo "[make] build host ak-flesh flasher"
	@echo "[make flash dev=/dev/ttyUSB0 FW_PATH=../../application/build_OLED_WATCH/OLED_WATCH.bin] flash app at $(APP_START_ADDR_VAL)"
	@echo "[make install] install ak-flesh and USB rules"

create:
	@echo mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.cpp
	@echo CXX $<
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@echo CXX $<
	@$(CC) -c -o $@ $< $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/$(MODULE): $(OBJ)
	@echo ---------- START LINK PROJECT ----------
	@echo $(CXX) -o $@ $^ $(CXXFLAGS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

flash:
	@echo $(OBJ_DIR)/$(MODULE) $(dev) $(FW_PATH) $(APP_START_ADDR_VAL) $(FLASH_MODE)
	@echo Flashing firmware to device $(dev) at address $(APP_START_ADDR_VAL) with mode $(FLASH_MODE)
	@$(OBJ_DIR)/$(MODULE) $(dev) $(FW_PATH) $(APP_START_ADDR_VAL) $(FLASH_MODE)

install: usb-rules
	@echo "Installing $(MODULE) to /usr/local/bin/..."
	@cp $(OBJ_DIR)/$(MODULE) /usr/local/bin/

usb-rules: 
	@echo "Installing USB rules for UART device..."
	@cp $(USB_RULES_FILE) /etc/udev/rules.d/
	@udevadm control --reload-rules
	@udevadm trigger

clean:
	@echo rm -rf $(OBJ_DIR)
	@rm -rf $(OBJ_DIR)
