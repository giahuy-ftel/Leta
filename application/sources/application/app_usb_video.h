#ifndef APP_USB_VIDEO_H
#define APP_USB_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define USB_VIDEO_FRAME_WIDTH 128U
#define USB_VIDEO_FRAME_HEIGHT 64U
#define USB_VIDEO_FRAME_SIZE (USB_VIDEO_FRAME_WIDTH * USB_VIDEO_FRAME_HEIGHT / 8U)
#define USB_VIDEO_REPORT_SIZE 64U

void app_usb_video_init(void);
void app_usb_video_set_frame_buffer(uint8_t *buffer);
void app_usb_video_set_active(bool active);
bool app_usb_video_active(void);
void app_usb_video_task(void);
bool app_usb_video_connected(void);
bool app_usb_video_frame_ready(void);
const uint8_t *app_usb_video_frame_buffer(void);

#ifdef __cplusplus
}
#endif

#endif
