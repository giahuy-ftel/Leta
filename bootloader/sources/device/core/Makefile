CFLAGS		+= -I./$(BOOTLOADER_SOURCES_DIR)/device/core
CPPFLAGS	+= -I./$(BOOTLOADER_SOURCES_DIR)/device/core

VPATH += $(BOOTLOADER_SOURCES_DIR)/device/core

SOURCES_ASM = $(BOOTLOADER_SOURCES_DIR)/device/core/startup_stm32f10x_md.s

LDFILE = $(BOOTLOADER_SOURCES_DIR)/device/core/STM32F103RBT6.ld

SOURCES += $(BOOTLOADER_SOURCES_DIR)/device/core/system_stm32f10x.c
# SOURCES += $(APP_SOURCES_DIR)/device/core/core_cm3.c
