Print = @echo

NAME_MODULE = LETA_BOOTLOADER
BOOTLOADER_SOURCES_DIR ?= sources
REPO_ROOT ?= $(abspath ..)

PROJECT = $(NAME_MODULE)
OBJECTS_DIR = build_$(NAME_MODULE)
TARGET = $(OBJECTS_DIR)/$(PROJECT).axf

FLASH_BASE_ADDR_VAL = 0x08000000
BOOTLOADER_FLASH_BYTES = 0x6000
SOC_SRAM_BYTES = 0x5000

include $(BOOTLOADER_SOURCES_DIR)/device/Makefile
include $(BOOTLOADER_SOURCES_DIR)/platform/Makefile
include $(BOOTLOADER_SOURCES_DIR)/driver/Makefile
include $(BOOTLOADER_SOURCES_DIR)/application/Makefile

TINYUSB_DIR = $(BOOTLOADER_SOURCES_DIR)/tinyusb
CFLAGS += -I./$(TINYUSB_DIR)/src
CPPFLAGS += -I./$(TINYUSB_DIR)/src
VPATH += $(TINYUSB_DIR)/src
VPATH += $(TINYUSB_DIR)/src/common
VPATH += $(TINYUSB_DIR)/src/device
VPATH += $(TINYUSB_DIR)/src/class/dfu
VPATH += $(TINYUSB_DIR)/src/portable/st/stm32_fsdev

SOURCES += $(TINYUSB_DIR)/src/tusb.c
SOURCES += $(TINYUSB_DIR)/src/common/tusb_fifo.c
SOURCES += $(TINYUSB_DIR)/src/device/usbd.c
SOURCES += $(TINYUSB_DIR)/src/class/dfu/dfu_device.c
SOURCES += $(TINYUSB_DIR)/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
SOURCES += $(TINYUSB_DIR)/src/portable/st/stm32_fsdev/fsdev_common.c

SOURCES_ = $(notdir $(SOURCES))
OBJECTS += $(addprefix $(OBJECTS_DIR)/, $(SOURCES_:.c=.o))

SOURCES_CPP_ = $(notdir $(SOURCES_CPP))
OBJECTS += $(addprefix $(OBJECTS_DIR)/, $(SOURCES_CPP_:.cpp=.o))

SOURCES_ASM_ = $(notdir $(SOURCES_ASM))
OBJECTS += $(addprefix $(OBJECTS_DIR)/, $(SOURCES_ASM_:.S=.o))
OBJECTS := $(OBJECTS:.s=.o)

OPTIMIZE_OPTION = -Os -g

TOOLS_DIR ?= $(REPO_ROOT)/tools
GCC_TOOL := $(shell command -v arm-none-eabi-gcc 2>/dev/null)
ifeq ($(strip $(GCC_TOOL)),)
GCC_PATH ?= $(TOOLS_DIR)/toolchain/gcc-arm-none-eabi-10.3-2021.10
else
GCC_PATH ?= $(patsubst %/bin/arm-none-eabi-gcc,%,$(GCC_TOOL))
endif

CC = $(GCC_PATH)/bin/arm-none-eabi-gcc
CPP = $(GCC_PATH)/bin/arm-none-eabi-g++
OBJCOPY = $(GCC_PATH)/bin/arm-none-eabi-objcopy
OBJDUMP = $(GCC_PATH)/bin/arm-none-eabi-objdump
OBJSIZE = $(GCC_PATH)/bin/arm-none-eabi-size
OBJNM = $(GCC_PATH)/bin/arm-none-eabi-nm

OPENOCD_PATH ?= openocd
OPENOCD_FLAGS ?= -f interface/stlink.cfg -f target/stm32f1x.cfg
OPENOCD_CMD ?= $(OPENOCD_PATH)
ST_FLASH ?= st-flash
DFU_UTIL ?= dfu-util
APP_BIN ?= ../application/build_OLED_WATCH/OLED_WATCH.bin

GCC_LIB_DIR := $(firstword $(wildcard $(GCC_PATH)/lib/gcc/arm-none-eabi/*/thumb/v7-m/nofp))
NEWLIB_DIR := $(GCC_PATH)/arm-none-eabi/lib/thumb/v7-m/nofp

LIB_PATH += -L$(NEWLIB_DIR)
LIB_PATH += -L$(GCC_LIB_DIR)

CPU = -mthumb -march=armv7-m
FPU = -mfloat-abi=soft

GENERAL_FLAGS += \
	$(OPTIMIZE_OPTION) \
	-DUSE_STDPERIPH_DRIVER \
	-DSTM32F10X_MD \
	-DSTM32F103xB \
	-DSTM32F1 \
	-DCFG_TUSB_MCU=OPT_MCU_STM32F1

AFLAGS += $(CPU) $(FPU) -MD

COMPILER_FLAGS += \
	$(CPU) \
	$(FPU) \
	-ffunction-sections \
	-fdata-sections \
	-fstack-usage \
	-MD \
	-Wall

CFLAGS += $(COMPILER_FLAGS) $(GENERAL_FLAGS) -std=gnu99
CPPFLAGS += $(COMPILER_FLAGS) $(GENERAL_FLAGS) -std=c++11 -fno-rtti -fno-exceptions -fno-use-cxa-atexit

LDFLAGS += \
	-nodefaultlibs \
	-nostartfiles \
	$(LIB_PATH) \
	-Wl,--defsym=FLASH_BASE_ADDR=$(FLASH_BASE_ADDR_VAL),--defsym=BOOTLOADER_FLASH_BYTES=$(BOOTLOADER_FLASH_BYTES) \
	-Wl,-T,$(LDFILE),-Map,$(OBJECTS_DIR)/$(PROJECT).map,--gc-sections,--print-memory-usage \
	$(CPU) $(FPU)

LIBS = $(NEWLIB_DIR)/libc_nano.a $(GCC_LIB_DIR)/libgcc.a

all: create $(TARGET)

create:
	$(Print) CREATE $(OBJECTS_DIR)
	@mkdir -p $(OBJECTS_DIR)

$(OBJECTS): | create

$(TARGET): $(OBJECTS) $(LIBS)
	$(Print) LD $@
	@$(CPP) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)
	$(Print) OBJCOPY $@
	@$(OBJCOPY) -O binary $@ $(@:.axf=.bin)
	@$(OBJCOPY) -O ihex $@ $(@:.axf=.hex)
	@$(OBJDUMP) -S -d $@ > $(@:.axf=.asm)
	@$(OBJSIZE) $@

$(OBJECTS_DIR)/%.o: %.c
	$(Print) CC $(notdir $@)
	@$(CC) -c $(CFLAGS) -o $@ $<

$(OBJECTS_DIR)/%.o: %.cpp
	$(Print) CXX $(notdir $@)
	@$(CPP) -c $(CPPFLAGS) -o $@ $<

$(OBJECTS_DIR)/%.o: %.S
	$(Print) AS $(notdir $@)
	@$(CC) $(AFLAGS) -o $@ -c $<

$(OBJECTS_DIR)/%.o: %.s
	$(Print) AS $(notdir $@)
	@$(CC) $(AFLAGS) -o $@ -c $<

.PHONY: clean flash-stlink flash-dfu reset asm sym

clean:
	$(Print) CLEAN $(OBJECTS_DIR)
	@rm -rf $(OBJECTS_DIR)

flash-stlink: $(TARGET)
	$(ST_FLASH) write $(TARGET:.axf=.bin) $(FLASH_BASE_ADDR_VAL) && $(ST_FLASH) reset

flash-dfu:
	$(DFU_UTIL) -d 0309:2001 -a 0 -D $(APP_BIN)

reset:
	$(OPENOCD_CMD) $(OPENOCD_FLAGS) -c init -c reset -c exit

asm: $(TARGET)
	@$(OBJDUMP) -D $(TARGET) > $(TARGET:.axf=.asm)

sym: $(TARGET)
	@$(OBJNM) --size-sort --print-size $(TARGET) > $(TARGET:.axf=.sym)
