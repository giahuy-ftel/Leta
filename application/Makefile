Print = @echo

NAME_MODULE = OLED_WATCH
APP_SOURCES_DIR ?= sources
REPO_ROOT ?= $(abspath ..)

APP_START_ADDR_VAL ?= 0x08007000
APP_VECTOR_OFFSET_VAL ?= 0x7000
PROJECT = $(NAME_MODULE)
OBJECTS_DIR = build_$(NAME_MODULE)

TARGET = $(OBJECTS_DIR)/$(PROJECT).axf

# App start address, synced with the linker script and interrupt vector table.
APP_ADDR_OPTION = -DAPP_START_ADDR=$(APP_START_ADDR_VAL)
VECT_TAB_OPTION = -DVECT_TAB_OFFSET=$(APP_VECTOR_OFFSET_VAL)

FLASH_BASE_ADDR_VAL = 0x08000000
SOC_FLASH_BYTES = 0x20000
SOC_SRAM_BYTES = 0x5000
MEMORY_USAGE_BAR_WIDTH = 40

##############################################################################
# Project files
##############################################################################
include $(APP_SOURCES_DIR)/device/Makefile
include $(APP_SOURCES_DIR)/platform/Makefile
include $(APP_SOURCES_DIR)/AKOS/Makefile
include $(APP_SOURCES_DIR)/driver/Makefile
include $(APP_SOURCES_DIR)/Oled_GUI/Makefile
include $(APP_SOURCES_DIR)/tinyusb/Makefile
include $(APP_SOURCES_DIR)/application/Makefile

SOURCES_ 		= $(notdir $(SOURCES))
OBJECTS 		+= $(addprefix $(OBJECTS_DIR)/, $(SOURCES_:.c=.o))

SOURCES_CPP_ 	= $(notdir $(SOURCES_CPP))
OBJECTS 		+= $(addprefix $(OBJECTS_DIR)/, $(SOURCES_CPP_:.cpp=.o))

SOURCES_ASM_ 	= $(notdir $(SOURCES_ASM))
OBJECTS			+= $(addprefix $(OBJECTS_DIR)/, $(SOURCES_ASM_:.S=.o))
OBJECTS			:= $(OBJECTS:.s=.o)

##############################################################################

#|---------------------------------------------------------------------------------------------------|
#| OPTIMIZE LEVELS                                                                                   |
#|------------|----------------------------------|--------------|---------|------------|-------------|
#|   option   | optimization levels              |execution time|code size|memory usage|complile time|
#|------------|----------------------------------|--------------|---------|------------|-------------|
#|   -O0      | compilation time                 |     (+)      |   (+)   |     (-)    |    (-)      |
#| -O1 || -O  | code size && execution time      |     (-)      |   (-)   |     (+)    |    (+)      |
#|   -O2      | more code size && execution time |     (--)     |         |     (+)    |    (++)     |
#|   -O3      | more code size && execution time |     (---)    |         |     (+)    |    (+++)    |
#|   -Os      | code size                        |              |   (--)  |            |    (++)     |
#|  -Ofast    | O3 with none math cals           |     (---)    |         |     (+)    |    (+++)    |
#|------------|----------------------------------|--------------|---------|------------|-------------|
OPTIMIZE_OPTION ?= -Os

TOOLS_DIR		?= $(REPO_ROOT)/tools
LOCAL_GCC_PATH		:= $(TOOLS_DIR)/toolchain/gcc-arm-none-eabi-10.3-2021.10
ifneq ($(wildcard $(LOCAL_GCC_PATH)/bin/arm-none-eabi-gcc),)
GCC_PATH		?= $(LOCAL_GCC_PATH)
else
GCC_TOOL		:= $(shell command -v arm-none-eabi-gcc 2>/dev/null)
ifeq ($(strip $(GCC_TOOL)),)
GCC_PATH		?= $(LOCAL_GCC_PATH)
else
GCC_PATH		?= $(patsubst %/bin/arm-none-eabi-gcc,%,$(GCC_TOOL))
endif
endif

OPENOCD_PATH	?= openocd
OPENOCD_FLAGS	?= -f interface/stlink.cfg -f target/stm32f1x.cfg
OPENOCD_CMD		?= $(OPENOCD_PATH)
ST_FLASH		?= st-flash

CC 			= $(GCC_PATH)/bin/arm-none-eabi-gcc
CPP 		= $(GCC_PATH)/bin/arm-none-eabi-g++
LD 			= $(GCC_PATH)/bin/arm-none-eabi-ld
AS 			= $(GCC_PATH)/bin/arm-none-eabi-as
STRIP 		= $(GCC_PATH)/bin/arm-none-eabi-strip
OBJCOPY 	= $(GCC_PATH)/bin/arm-none-eabi-objcopy
OBJDUMP 	= $(GCC_PATH)/bin/arm-none-eabi-objdump
OBJSIZE 	= $(GCC_PATH)/bin/arm-none-eabi-size
OBJNM 		= $(GCC_PATH)/bin/arm-none-eabi-nm
GDB			= $(GCC_PATH)/bin/arm-none-eabi-gdb

GCC_LIB_DIR := $(firstword $(wildcard $(GCC_PATH)/lib/gcc/arm-none-eabi/*/thumb/v7-m/nofp))
NEWLIB_DIR := $(GCC_PATH)/arm-none-eabi/lib/thumb/v7-m/nofp

LIBC		= $(NEWLIB_DIR)/libc.a
LIBM		= $(NEWLIB_DIR)/libm.a
LIBG		= $(NEWLIB_DIR)/libg.a
LIBRDPMON	= $(NEWLIB_DIR)/librdpmon.a
LIBSTDCPP_NANO	= $(NEWLIB_DIR)/libstdc++_nano.a
LIBGCC		= $(GCC_LIB_DIR)/libgcc.a
LIBGCOV		= $(GCC_LIB_DIR)/libgcov.a

LIB_PATH += -L$(NEWLIB_DIR)
LIB_PATH += -L$(GCC_LIB_DIR)

CPU = -mthumb -march=armv7-m
FPU = -mfloat-abi=soft

GENERAL_FLAGS +=			\
			$(OPTIMIZE_OPTION)	\
			$(APP_ADDR_OPTION)	\
			$(VECT_TAB_OPTION)	\
			-DUSE_STDPERIPH_DRIVER	\
			-DSTM32F10X_MD		\
			-DSTM32F103xB		\
			-DSTM32F1

AFLAGS +=	$(CPU)	\
		$(FPU)	\
		-MD

COMPILER_FLAGS +=			\
		$(CPU)			\
		$(FPU)			\
		-ffunction-sections	\
		-fdata-sections		\
		-fstack-usage		\
		-MD			\
		-Wall

CFLAGS += $(COMPILER_FLAGS) $(GENERAL_FLAGS) -std=gnu99

CPPFLAGS +=					\
		$(COMPILER_FLAGS)	\
		$(GENERAL_FLAGS)	\
		-std=c++11		\
		-fno-rtti		\
		-fno-exceptions		\
		-fno-use-cxa-atexit

LDFLAGS += 	-lgcc \
			-nodefaultlibs \
			-nostartfiles \
			$(LIB_PATH) \
			-Wl,--defsym=FLASH_BASE_ADDR=$(FLASH_BASE_ADDR_VAL),--defsym=SOC_FLASH_BYTES=$(SOC_FLASH_BYTES),--defsym=APP_START_ADDR=$(APP_START_ADDR_VAL) \
			-Wl,-T,$(LDFILE),-Map,$(OBJECTS_DIR)/$(PROJECT).map,--gc-sections,--print-memory-usage \
			$(CPU) $(FPU)

LIBS = $(LIBC) $(LIBM) $(LIBSTDCPP_NANO) $(LIBGCC) $(LIBGCOV) $(LIBG) $(LIBRDPMON)

define MEMORY_INFO_COLLECT
	FLASH_USED=`$(OBJSIZE) -A -d $(TARGET) | awk '$$1 == ".isr_vector" || $$1 == ".text" || $$1 == ".rodata" || $$1 == ".data" || $$1 == ".preinit_array" || $$1 == ".init_array" || $$1 == ".fini_array" || $$1 == ".task_desc" || $$1 == ".ARM" {sum += $$2} END {print sum+0}'`; \
	RAM_USED=`$(OBJSIZE) -A -d $(TARGET) | awk '$$1 == ".data" || $$1 == ".bss" || $$1 == "._user_heap_stack" {sum += $$2} END {print sum+0}'`; \
	APP_FLASH_BYTES=$$(( $(SOC_FLASH_BYTES) - ($(APP_START_ADDR_VAL) - $(FLASH_BASE_ADDR_VAL)) )); \
	FLASH_USED_PCT=`awk "BEGIN {printf \"%.2f\", ($$FLASH_USED * 100.0) / $$APP_FLASH_BYTES}"`; \
	RAM_USED_PCT=`awk "BEGIN {printf \"%.2f\", ($$RAM_USED * 100.0) / $(SOC_SRAM_BYTES)}"`; \
	FLASH_USED_BAR=`awk "BEGIN {printf \"%d\", ($$FLASH_USED * $(MEMORY_USAGE_BAR_WIDTH)) / $$APP_FLASH_BYTES}"`; \
	FLASH_FREE_BAR=$$(( $(MEMORY_USAGE_BAR_WIDTH) - $$FLASH_USED_BAR )); \
	RAM_USED_BAR=`awk "BEGIN {printf \"%d\", ($$RAM_USED * $(MEMORY_USAGE_BAR_WIDTH)) / $(SOC_SRAM_BYTES)}"`; \
	RAM_FREE_BAR=$$(( $(MEMORY_USAGE_BAR_WIDTH) - $$RAM_USED_BAR )); \
	FLASH_USED_BLOCKS=`printf "%*s" $$FLASH_USED_BAR "" | tr ' ' '#'`; \
	FLASH_FREE_BLOCKS=`printf "%*s" $$FLASH_FREE_BAR "" | tr ' ' '-'`; \
	RAM_USED_BLOCKS=`printf "%*s" $$RAM_USED_BAR "" | tr ' ' '#'`; \
	RAM_FREE_BLOCKS=`printf "%*s" $$RAM_FREE_BAR "" | tr ' ' '-'`; \
	echo "FLASH: [$$FLASH_USED_BLOCKS$$FLASH_FREE_BLOCKS] $$FLASH_USED / $$APP_FLASH_BYTES bytes ($$FLASH_USED_PCT% used)"; \
	echo "RAM  : [$$RAM_USED_BLOCKS$$RAM_FREE_BLOCKS] $$RAM_USED / $(SOC_SRAM_BYTES) bytes ($$RAM_USED_PCT% used)"
endef

all: create $(TARGET)

create:
	$(Print) "           )\     *****************************"
	$(Print) "  c=======<||)    " CREATE $(OBJECTS_DIR) folder
	$(Print) "           )(     *****************************"
	@mkdir -p $(OBJECTS_DIR)

$(OBJECTS): | create

$(TARGET): $(OBJECTS) $(LIBS)
	$(Print) LD $@
	@$(CPP) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)
	$(Print) OBJCOPY $(@)
	@$(OBJCOPY) -O binary $(@) $(@:.axf=.bin)
	@$(OBJCOPY) -O ihex $(@) $(@:.axf=.hex)
	@$(OBJDUMP) -S -d $(@) > $(@:.axf=.asm)
	@$(OBJSIZE) $(TARGET)
	@$(call MEMORY_INFO_COLLECT)

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

.PHONY: help clean debug com flash-stlink erase verify reset asm sym

help:
	$(Print) "[make] build app image at 0x08007000"
	$(Print) "[make flash-stlink] burn relocated app via st-link at 0x08007000"
	$(Print) "[make com dev=/dev/tty*] console"
	$(Print) "[make debug] gdb debug"
	$(Print) "[make clean] clean build project folder"
	$(Print) "[make asm] generate asm file"
	$(Print) "[make sym] list symbols from object file"

clean:
	$(Print) "        __         *****************************"
	$(Print) "      __\ \___     " CLEAN $(OBJECTS_DIR) folder
	$(Print) "      \ _ _ _ \    "
	$(Print) "       \_\_\_\_\   *****************************"
	@rm -rf $(OBJECTS_DIR)

debug:
	$(GDB) $(TARGET) -ex "target remote localhost:3333"

com:
ifdef dev
	minicom -D $(dev) -b 115200 -c on
else
	$(Print) "Example: make com dev=/dev/ttyUSB0"
endif

flash-stlink: $(TARGET)
	$(ST_FLASH) write $(TARGET:.axf=.bin) $(APP_START_ADDR_VAL) && $(ST_FLASH) reset

erase:
	$(OPENOCD_CMD) $(OPENOCD_FLAGS) -c init -c halt -c "flash erase_sector 0 0 last" -c reset -c exit

verify: $(TARGET)
	$(OPENOCD_CMD) $(OPENOCD_FLAGS) -c init -c halt -c "verify_image $(TARGET:.axf=.hex)" -c exit

reset:
	$(OPENOCD_CMD) $(OPENOCD_FLAGS) -c init -c reset -c exit

asm: $(TARGET)
	$(Print) disassemble-all $(TARGET:.axf=.asm)
	@$(OBJDUMP) -D $(TARGET) > $(TARGET:.axf=.asm)

sym: $(TARGET)
	$(Print) export object name $(TARGET:.axf=.sym)
	@$(OBJNM) --size-sort --print-size $(TARGET) > $(TARGET:.axf=.sym)
