#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

#define BOOT_DFU_POWER_OFF_TIMEOUT_MS   120000UL
#define BOOT_DFU_MANIFEST_GRACE_MS      1000UL
#define BOOT_DFU_BUTTON_WINDOW_MS       1000U
#define BOOT_DFU_BUTTON_STABLE_MS       50U

#define BOOTLOADER_START_ADDR           0x08000000UL
#define BOOTLOADER_END_ADDR             0x08006000UL
#define BOOT_STATE_ADDR                 0x08006000UL
#define BOOT_STATE_END_ADDR             0x08007000UL
#define APP_START_ADDR                  0x08007000UL
#define FLASH_END_ADDR                  0x08020000UL
#define FLASH_PAGE_SIZE                 1024UL

#endif
