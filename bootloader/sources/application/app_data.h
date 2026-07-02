#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>

#define APP_DATA_PACKED                    __attribute__((__packed__))

#define FIRMWARE_PSK                       0x1A2B3C4D

#define SYS_BOOT_CMD_UPDATE_NONE           0x01
#define SYS_BOOT_CMD_UPDATE_REQ            0x02
#define SYS_BOOT_CMD_UPDATE_RES            0x03

#define SYS_BOOT_CONTAINER_DIRECTLY        0x01
#define SYS_BOOT_CONTAINER_EXTERNAL_FLASH  0x02
#define SYS_BOOT_CONTAINER_INTERNAL_FLASH  0x03
#define SYS_BOOT_CONTAINER_EXTERNAL_EPPROM 0x04
#define SYS_BOOT_CONTAINER_INTERNAL_EPPROM 0x05
#define SYS_BOOT_CONTAINER_SDCARD          0x06

#define SYS_BOOT_IO_DRIVER_NONE            0x01
#define SYS_BOOT_IO_DRIVER_UART            0x02
#define SYS_BOOT_IO_DRIVER_SPI             0x03

#define SYS_BOOT_RETURN_NONE               0x00
#define SYS_BOOT_RETURN_BOOTLOADER         0x01
#define SYS_BOOT_RETURN_LAUNCHER           0x02

typedef struct {
	uint32_t psk;
	uint32_t bin_len;
	uint16_t checksum;
} firmware_header_t;

typedef struct {
	uint8_t type;
	uint8_t src_task_id;
	uint8_t des_task_id;
	uint8_t sig;
	uint8_t if_src_type;
	uint8_t if_des_type;
} APP_DATA_PACKED ak_msg_host_res_t;

typedef struct {
	uint8_t cmd;
	uint8_t container;
	uint8_t io_driver;
	uint32_t des_addr;
	uint32_t src_addr;
	ak_msg_host_res_t ak_msg_res;
} firmware_boot_cmd_t;

typedef struct {
	firmware_header_t current_fw_boot_header;
	firmware_header_t current_fw_app_header;
	firmware_header_t update_fw_boot_header;
	firmware_header_t update_fw_app_header;
	firmware_boot_cmd_t fw_boot_cmd;
	firmware_boot_cmd_t fw_app_cmd;
	uint8_t return_mode;
	uint8_t reserved[3];
} sys_boot_t;

#endif
