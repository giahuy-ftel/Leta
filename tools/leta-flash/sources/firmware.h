#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <stdint.h>
#include <string>
#include <vector>

#define LETA_APP_START_ADDR 0x08007000UL
#define LETA_DFU_IMAGE_MAGIC 0x4154454CUL
#define LETA_DFU_IMAGE_HEADER_VERSION 1U

typedef struct {
	uint32_t magic;
	uint16_t header_version;
	uint16_t header_size;
	uint32_t app_addr;
	uint32_t app_len;
	uint32_t app_crc32;
} leta_dfu_image_header_t;

bool firmware_read_file(const std::string &path, std::vector<uint8_t> *data);
uint32_t firmware_crc32(const uint8_t *data, uint32_t length);
bool firmware_write_dfu_container(const std::string &app_path, const std::string &dfu_path,
		leta_dfu_image_header_t *header);

#endif
