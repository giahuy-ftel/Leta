#include "firmware.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_dbg.h"

bool firmware_read_file(const std::string &path, std::vector<uint8_t> *data)
{
	struct stat file_info;
	int fd = open(path.c_str(), O_RDONLY);

	if (fd < 0) {
		APP_DBG(DEBUG_LEVEL_ERROR, "open failed: %s errno=%d (%s)\n",
				path.c_str(), errno, strerror(errno));
		return false;
	}

	if (fstat(fd, &file_info) < 0) {
		APP_DBG(DEBUG_LEVEL_ERROR, "fstat failed: %s errno=%d (%s)\n",
				path.c_str(), errno, strerror(errno));
		close(fd);
		return false;
	}

	if (file_info.st_size <= 0) {
		APP_DBG(DEBUG_LEVEL_ERROR, "firmware is empty: %s\n", path.c_str());
		close(fd);
		return false;
	}

	data->resize((size_t)file_info.st_size);
	ssize_t read_len = read(fd, data->data(), data->size());
	close(fd);

	if (read_len != (ssize_t)data->size()) {
		APP_DBG(DEBUG_LEVEL_ERROR, "read failed: %s errno=%d (%s)\n",
				path.c_str(), errno, strerror(errno));
		return false;
	}

	return true;
}

uint32_t firmware_crc32(const uint8_t *data, uint32_t length)
{
	uint32_t crc = 0;
	crc = ~crc;

	for (uint32_t i = 0; i < length; i++) {
		crc ^= data[i];

		for (uint32_t bit = 0; bit < 8; bit++) {
			if (crc & 1U) {
				crc = (crc >> 1) ^ 0xEDB88320UL;
			} else {
				crc >>= 1;
			}
		}
	}

	return ~crc;
}

bool firmware_write_dfu_container(const std::string &app_path, const std::string &dfu_path,
		leta_dfu_image_header_t *header)
{
	std::vector<uint8_t> app;

	if (!firmware_read_file(app_path, &app)) {
		return false;
	}

	header->magic = LETA_DFU_IMAGE_MAGIC;
	header->header_version = LETA_DFU_IMAGE_HEADER_VERSION;
	header->header_size = sizeof(*header);
	header->app_addr = LETA_APP_START_ADDR;
	header->app_len = app.size();
	header->app_crc32 = firmware_crc32(app.data(), app.size());

	int fd = open(dfu_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0) {
		APP_DBG(DEBUG_LEVEL_ERROR, "create failed: %s errno=%d (%s)\n",
				dfu_path.c_str(), errno, strerror(errno));
		return false;
	}

	if (write(fd, header, sizeof(*header)) != (ssize_t)sizeof(*header) ||
			write(fd, app.data(), app.size()) != (ssize_t)app.size()) {
		APP_DBG(DEBUG_LEVEL_ERROR, "write failed: %s errno=%d (%s)\n",
				dfu_path.c_str(), errno, strerror(errno));
		close(fd);
		return false;
	}

	close(fd);
	return true;
}
