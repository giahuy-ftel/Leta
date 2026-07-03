#include "app_usb_video.h"

#include <string.h>

#include "stm32f10x.h"
#include "tusb.h"

enum {
	ITF_NUM_HID,
	ITF_NUM_TOTAL
};

enum {
	STRID_LANGID = 0,
	STRID_MANUFACTURER,
	STRID_PRODUCT,
	STRID_HID
};

#define USB_VID 0x0309
#define USB_PID 0x2002
#define USB_BCD 0x0100
#define EPNUM_HID_OUT 0x01
#define EPNUM_HID_IN 0x81
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static bool usb_video_started;
static volatile bool usb_video_active;
static uint8_t *frame_buffer;
static uint16_t frame_write_offset;
static volatile bool frame_ready;

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

static const uint8_t desc_hid_report[] = {
	TUD_HID_REPORT_DESC_GENERIC_INOUT(USB_VIDEO_REPORT_SIZE)
};

static const uint8_t desc_configuration[] = {
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),
	TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, STRID_HID, HID_ITF_PROTOCOL_NONE,
			sizeof(desc_hid_report), EPNUM_HID_OUT, EPNUM_HID_IN,
			CFG_TUD_HID_EP_BUFSIZE, 1),
};

static const char *string_desc_arr[] = {
	NULL,
	"Leta",
	"USB Video",
	"OLED Frame HID"
};

static uint16_t desc_str[32 + 1];

static void usb_clock_init(void)
{
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

void app_usb_video_init(void)
{
	if (usb_video_started) {
		return;
	}

	if (frame_buffer != NULL) {
		memset(frame_buffer, 0, USB_VIDEO_FRAME_SIZE);
	}
	frame_write_offset = 0;
	frame_ready = false;

	usb_clock_init();

	tusb_rhport_init_t dev_init = {
		.role = TUSB_ROLE_DEVICE,
		.speed = TUSB_SPEED_AUTO
	};
	tusb_init(0, &dev_init);
	usb_video_started = true;
}

void app_usb_video_set_frame_buffer(uint8_t *buffer)
{
	frame_buffer = buffer;
	frame_write_offset = 0;
	frame_ready = false;
}

static void usb_video_write_frame_chunk(uint8_t const *buffer, uint16_t bufsize)
{
	if (frame_buffer == NULL || bufsize == 0) {
		return;
	}

	if (bufsize > (USB_VIDEO_FRAME_SIZE - frame_write_offset)) {
		frame_write_offset = 0;
	}

	memcpy(&frame_buffer[frame_write_offset], buffer, bufsize);
	frame_write_offset += bufsize;

	if (frame_write_offset >= USB_VIDEO_FRAME_SIZE) {
		frame_write_offset = 0;
		frame_ready = true;
	}
}

void app_usb_video_set_active(bool active)
{
	usb_video_active = active;
}

bool app_usb_video_active(void)
{
	return usb_video_active;
}

void app_usb_video_task(void)
{
	if (usb_video_started) {
		tud_task();
	}
}

bool app_usb_video_connected(void)
{
	return usb_video_started && tud_ready();
}

bool app_usb_video_frame_ready(void)
{
	bool ready = frame_ready;

	frame_ready = false;
	return ready;
}

const uint8_t *app_usb_video_frame_buffer(void)
{
	return frame_buffer;
}

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
		desc_str[1] = 0x0409;
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

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
	(void)instance;
	return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
		hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;
	return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
		hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
	(void)instance;

	if (report_type != HID_REPORT_TYPE_OUTPUT || bufsize == 0) {
		return;
	}

	if (report_id == 0 && bufsize == (USB_VIDEO_REPORT_SIZE + 1U) && buffer[0] == 0) {
		buffer++;
		bufsize--;
	}

	usb_video_write_frame_chunk(buffer, bufsize);
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
	dcd_int_handler(0);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	dcd_int_handler(0);
}
