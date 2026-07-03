##############################################################################
# Leta USB Video streamer
##############################################################################

MODULE = leta-video
USB_RULES_FILE = usb_rules/leta_usb_video.rules

OPTIMIZE = -g -Os
CXX = g++
OBJ_DIR = build_$(MODULE)

CXXFLAGS += $(OPTIMIZE) \
	-std=c++11 \
	-Wall \
	-Winline \
	-pipe

LDFLAGS += -lX11

.PHONY: help all install usb-rules clean

all: create $(OBJ_DIR)/$(MODULE)

help:
	@echo "[make] build host leta-video USB HID screen streamer"
	@echo "[make install] install leta-video and USB Video rules"

create:
	@echo mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/$(MODULE): sources/leta_video.cpp | create
	@echo CXX $<
	@$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

install: $(OBJ_DIR)/$(MODULE) usb-rules
	@echo "Installing $(MODULE) to /usr/local/bin/..."
	@cp $(OBJ_DIR)/$(MODULE) /usr/local/bin/

usb-rules:
	@echo "Installing USB Video rules..."
	@cp $(USB_RULES_FILE) /etc/udev/rules.d/
	@udevadm control --reload-rules
	@udevadm trigger

clean:
	@echo rm -rf $(OBJ_DIR)
	@rm -rf $(OBJ_DIR)
