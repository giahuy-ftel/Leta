#include "boot_dfu.h"

#include <stdint.h>
#include <string.h>

#include "app_data.h"
#include "boot_oled.h"
#include "boot_share.h"
#include "main.h"
#include "stm32f1xx.h"
#include "system.h"
#include "system_stm32f10x.h"
#include "tusb.h"

#ifndef DFU_STATUS_ERR_VERIFY
#define DFU_STATUS_ERR_VERIFY 0x07U
#endif

#define DFU_IMAGE_MAGIC             0x4154454CUL
#define DFU_IMAGE_HEADER_VERSION    1U

typedef struct {
	uint32_t magic;
	uint16_t header_version;
	uint16_t header_size;
	uint32_t app_addr;
	uint32_t app_len;
	uint32_t app_crc32;
} dfu_image_header_t;

static uint32_t dfu_last_activity_ticks;
static uint32_t dfu_written_end_addr;
static bool dfu_update_inprogress_marked;
static bool manifest_done;
static dfu_image_header_t dfu_image_header;
static uint32_t dfu_header_received;
static bool dfu_header_ready;
static uint8_t dfu_display_percent = 0xFFU;

static void usb_clock_init(void)
{
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

static void dfu_touch(void)
{
	dfu_last_activity_ticks = system_millis();
}

static void dfu_show_progress(uint8_t percent)
{
	if (percent != dfu_display_percent) {
		dfu_display_percent = percent;
		bootloader_oled_show_dfu_progress(percent);
	}
}

static bool dfu_range_valid(uint32_t address, uint32_t length)
{
	if (length == 0) {
		return true;
	}

	if ((address < APP_START_ADDR) || (address >= FLASH_END_ADDR)) {
		return false;
	}

	return length <= (FLASH_END_ADDR - address);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t length)
{
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

static uint32_t boot_app_crc32(uint32_t image_length)
{
	uint32_t crc = 0;
	uint8_t buffer[64];

	for (uint32_t offset = 0; offset < image_length; offset += sizeof(buffer)) {
		uint32_t read_len = image_length - offset;

		if (read_len > sizeof(buffer)) {
			read_len = sizeof(buffer);
		}

		memcpy(buffer, (const void *)(APP_START_ADDR + offset), read_len);
		crc = crc32_update(crc, buffer, read_len);
	}

	return crc;
}

static bool dfu_header_valid(const dfu_image_header_t *header)
{
	if (header->magic != DFU_IMAGE_MAGIC) {
		return false;
	}

	if (header->header_version != DFU_IMAGE_HEADER_VERSION ||
			header->header_size != sizeof(*header)) {
		return false;
	}

	if (header->app_addr != APP_START_ADDR ||
			header->app_len == 0 ||
			header->app_len > (FLASH_END_ADDR - APP_START_ADDR)) {
		return false;
	}

	return true;
}

static void dfu_receive_header(uint32_t image_offset, const uint8_t *data, uint32_t length)
{
	if (dfu_header_ready || image_offset >= sizeof(dfu_image_header)) {
		return;
	}

	uint32_t copy_len = sizeof(dfu_image_header) - image_offset;
	if (copy_len > length) {
		copy_len = length;
	}

	memcpy(((uint8_t *)&dfu_image_header) + image_offset, data, copy_len);
	if ((image_offset + copy_len) > dfu_header_received) {
		dfu_header_received = image_offset + copy_len;
	}

	if (dfu_header_received >= sizeof(dfu_image_header)) {
		dfu_header_ready = dfu_header_valid(&dfu_image_header);
	}
}

static bool dfu_payload_from_block(uint32_t image_offset, const uint8_t *data, uint32_t length,
		const uint8_t **payload, uint32_t *payload_addr, uint32_t *payload_len)
{
	uint32_t payload_offset;

	*payload = NULL;
	*payload_addr = APP_START_ADDR;
	*payload_len = 0;

	if (length == 0) {
		return true;
	}

	dfu_receive_header(image_offset, data, length);

	if (!dfu_header_ready) {
		return dfu_header_received < sizeof(dfu_image_header);
	}

	if (image_offset >= dfu_image_header.header_size) {
		payload_offset = image_offset - dfu_image_header.header_size;
	} else {
		uint32_t header_bytes = dfu_image_header.header_size - image_offset;

		if (header_bytes >= length) {
			return true;
		}

		data += header_bytes;
		length -= header_bytes;
		payload_offset = 0;
	}

	if (payload_offset >= dfu_image_header.app_len) {
		return length == 0;
	}

	if (length > (dfu_image_header.app_len - payload_offset)) {
		return false;
	}

	*payload = data;
	*payload_addr = APP_START_ADDR + payload_offset;
	*payload_len = length;
	return true;
}

static void boot_mark_dfu_manifest_done(void)
{
	sys_boot_t boot;

	boot_share_get(&boot);

	memset(&boot, 0xFF, sizeof(boot));
	boot.current_fw_app_header.psk = FIRMWARE_PSK;
	boot.current_fw_app_header.bin_len = dfu_image_header.app_len;
	boot.current_fw_app_header.crc32 = dfu_image_header.app_crc32;
	boot.fw_app_cmd.cmd = SYS_BOOT_CMD_UPDATE_NONE;
	boot.fw_app_cmd.container = SYS_BOOT_CONTAINER_DIRECTLY;
	boot.return_mode = SYS_BOOT_RETURN_NONE;

	(void)boot_share_set(&boot);
}

static void boot_mark_dfu_update_inprogress(void)
{
	sys_boot_t boot;

	if (dfu_update_inprogress_marked) {
		return;
	}

	boot_share_get(&boot);
	if (boot_share_erased_or_invalid(&boot)) {
		memset(&boot, 0xFF, sizeof(boot));
	}

	boot.fw_app_cmd.cmd = SYS_BOOT_CMD_UPDATE_INPROGRESS;
	boot.fw_app_cmd.container = SYS_BOOT_CONTAINER_DIRECTLY;
	boot.return_mode = SYS_BOOT_RETURN_NONE;

	(void)boot_share_set(&boot);
	dfu_update_inprogress_marked = true;
}

void boot_dfu_enter(void)
{
	bootloader_oled_show_dfu_mode();

	system_buttons_init();
	SysTick_Config(SystemCoreClock / 1000UL);
	dfu_touch();
	usb_clock_init();

	tusb_rhport_init_t dev_init = {
		.role = TUSB_ROLE_DEVICE,
		.speed = TUSB_SPEED_AUTO
	};
	tusb_init(0, &dev_init);
}

void boot_dfu_task(void)
{
	tud_task();
}

bool boot_dfu_manifest_done(void)
{
	return manifest_done;
}

bool boot_dfu_timed_out(void)
{
	return (system_millis() - dfu_last_activity_ticks) >= BOOT_DFU_POWER_OFF_TIMEOUT_MS;
}

uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state)
{
	(void)alt;

	if (state == DFU_DNBUSY) {
		return 50;
	}

	return 0;
}

void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, const uint8_t *data, uint16_t length)
{
	(void)alt;
	dfu_touch();

	uint32_t image_offset = (uint32_t)block_num * CFG_TUD_DFU_XFER_BUFSIZE;
	const uint8_t *payload;
	uint32_t address;
	uint32_t payload_len;
	uint8_t status = DFU_STATUS_OK;

	if (!dfu_payload_from_block(image_offset, data, length, &payload, &address, &payload_len) ||
			!dfu_range_valid(address, payload_len)) {
		tud_dfu_finish_flashing(DFU_STATUS_ERR_ADDRESS);
		return;
	}

	if (payload_len > 0) {
		boot_mark_dfu_update_inprogress();
	}

	system_flash_unlock();

	for (uint32_t offset = 0; (status == DFU_STATUS_OK) && (offset < payload_len); offset += 2) {
		uint32_t write_addr = address + offset;
		uint16_t halfword = payload[offset];

		if ((write_addr % FLASH_PAGE_SIZE) == 0) {
			if (!system_flash_erase_page(write_addr)) {
				status = DFU_STATUS_ERR_ERASE;
				break;
			}
		}

		if ((offset + 1) < payload_len) {
			halfword |= ((uint16_t)payload[offset + 1] << 8);
		} else {
			halfword |= 0xFF00U;
		}

		if (!system_flash_program_halfword(write_addr, halfword)) {
			status = DFU_STATUS_ERR_PROG;
		}
	}

	if ((status == DFU_STATUS_OK) && (payload_len > 0)) {
		uint32_t written_end = address + payload_len;
		if (written_end > dfu_written_end_addr) {
			dfu_written_end_addr = written_end;
		}

		if (dfu_header_ready && dfu_image_header.app_len > 0) {
			uint32_t written_len = dfu_written_end_addr - APP_START_ADDR;
			if (written_len > dfu_image_header.app_len) {
				written_len = dfu_image_header.app_len;
			}
			dfu_show_progress((uint8_t)((written_len * 100UL) / dfu_image_header.app_len));
		}
	}

	system_flash_lock();
	tud_dfu_finish_flashing(status);
}

uint16_t tud_dfu_upload_cb(uint8_t alt, uint16_t block_num, uint8_t *data, uint16_t length)
{
	(void)alt;
	dfu_touch();

	uint32_t address = APP_START_ADDR + ((uint32_t)block_num * CFG_TUD_DFU_XFER_BUFSIZE);
	if (!dfu_range_valid(address, length)) {
		return 0;
	}

	memcpy(data, (const void *)address, length);
	return length;
}

void tud_dfu_manifest_cb(uint8_t alt)
{
	(void)alt;
	dfu_touch();

	if (!dfu_header_ready ||
			dfu_written_end_addr != (APP_START_ADDR + dfu_image_header.app_len) ||
			boot_app_crc32(dfu_image_header.app_len) != dfu_image_header.app_crc32) {
		tud_dfu_finish_flashing(DFU_STATUS_ERR_VERIFY);
		return;
	}

	boot_mark_dfu_manifest_done();
	manifest_done = true;
	dfu_display_percent = 100;
	bootloader_oled_show_dfu_done();
	tud_dfu_finish_flashing(DFU_STATUS_OK);
}

void tud_dfu_abort_cb(uint8_t alt)
{
	(void)alt;
	dfu_touch();
}

void tud_dfu_detach_cb(void)
{
	dfu_touch();
	NVIC_SystemReset();
}
