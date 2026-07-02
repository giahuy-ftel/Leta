#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>

#define FIRMWARE_PSK                       0x1A2B3C4D

#define SYS_BOOT_CMD_UPDATE_NONE           0x01
#define SYS_BOOT_CMD_UPDATE_INPROGRESS     0x02

#define SYS_BOOT_CONTAINER_DIRECTLY        0x01

#define SYS_BOOT_RETURN_NONE               0x00
#define SYS_BOOT_RETURN_BOOTLOADER         0x01

typedef struct {
	uint32_t psk;
	uint32_t bin_len;
	uint32_t crc32;
} firmware_header_t;

typedef struct {
	uint8_t cmd;
	uint8_t container;
	uint8_t reserved[2];
} firmware_boot_cmd_t;

typedef struct {
	firmware_header_t current_fw_app_header;
	firmware_boot_cmd_t fw_app_cmd;
	uint8_t return_mode;
	uint8_t reserved[3];
} sys_boot_t;

#endif
