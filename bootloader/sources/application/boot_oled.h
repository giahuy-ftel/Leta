#ifndef __BOOT_OLED_H__
#define __BOOT_OLED_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bootloader_oled_show_dfu_mode(void);
void bootloader_oled_show_dfu_progress(uint8_t percent);
void bootloader_oled_show_dfu_done(void);
void bootloader_oled_show_usb_icon(void);
void bootloader_oled_display_off(void);

#ifdef __cplusplus
}
#endif

#endif
