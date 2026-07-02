#include <stdio.h>
#include <string.h>

#include "app_dbg.h"
#include "firmware.h"
#include "usb_dfu.h"

static const char *DEFAULT_DFU_PATH = "build_leta-flash/firmware.dfu";

static void print_usage(const char *program)
{
	printf("Usage:\n");
	printf("  %s pack <app.bin> [out.dfu]\n", program);
	printf("  %s flash <app.bin> [out.dfu]\n", program);
}

static bool pack_firmware(const char *app_path, const char *dfu_path)
{
	leta_dfu_image_header_t header;

	if (!firmware_write_dfu_container(app_path, dfu_path, &header)) {
		return false;
	}

	APP_DBG(DEBUG_LEVEL_INFO, "created %s\n", dfu_path);
	APP_DBG(DEBUG_LEVEL_INFO, "app_addr=0x%08X app_len=%u crc32=0x%08X header_size=%u\n",
			header.app_addr, header.app_len, header.app_crc32, header.header_size);

	return true;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		print_usage(argv[0]);
		return 1;
	}

	const char *command = argv[1];
	const char *app_path = argv[2];
	const char *dfu_path = (argc >= 4) ? argv[3] : DEFAULT_DFU_PATH;

	if (strcmp(command, "pack") == 0) {
		return pack_firmware(app_path, dfu_path) ? 0 : 1;
	}

	if (strcmp(command, "flash") == 0) {
		if (!pack_firmware(app_path, dfu_path)) {
			return 1;
		}

		return usb_dfu_flash(dfu_path) ? 0 : 2;
	}

	print_usage(argv[0]);
	return 1;
}
