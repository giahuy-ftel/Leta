#include <stdint.h>
#include <string.h>
#include "tusb.h"

enum {
	ITF_NUM_DFU,
	ITF_NUM_TOTAL
};

enum {
	STRID_LANGID = 0,
	STRID_MANUFACTURER,
	STRID_PRODUCT
};

#define USB_VID 0x0309
#define USB_PID 0x2001
#define USB_BCD 0x0100
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_DFU_DESC_LEN(1))
#define DFU_ATTRS (DFU_ATTR_CAN_UPLOAD | DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_MANIFESTATION_TOLERANT)

static const tusb_desc_device_t desc_device = {
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = USB_BCD,
	.iManufacturer = STRID_MANUFACTURER,
	.iProduct = STRID_PRODUCT,
	.iSerialNumber = 0,
	.bNumConfigurations = 0x01
};

static const uint8_t desc_configuration[] = {
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),
	TUD_DFU_DESCRIPTOR(ITF_NUM_DFU, 1, STRID_PRODUCT, DFU_ATTRS, 1000, CFG_TUD_DFU_XFER_BUFSIZE),
};

static const char *string_desc_arr[] = {
	(const char[]){0x09, 0x04},
	"Leta",
	"USB DFU Bootloader"
};

static uint16_t desc_str[32 + 1];

const uint8_t *tud_descriptor_device_cb(void)
{
	return (const uint8_t *)&desc_device;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
	(void)index;
	return desc_configuration;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	(void)langid;

	size_t chr_count;
	if (index == STRID_LANGID) {
		memcpy(&desc_str[1], string_desc_arr[0], 2);
		chr_count = 1;
	} else {
		if (index >= (sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
			return NULL;
		}

		const char *str = string_desc_arr[index];
		chr_count = strlen(str);
		if (chr_count > 32) {
			chr_count = 32;
		}

		for (size_t i = 0; i < chr_count; i++) {
			desc_str[1 + i] = (uint8_t)str[i];
		}
	}

	desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
	return desc_str;
}
