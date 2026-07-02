#ifndef __SH1106_H__
#define __SH1106_H__

#include <stdbool.h>
#include <stdint.h>

#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define SH1106_LCDWIDTH          128
#define SH1106_LCDHEIGHT         64
#define SH1106_MAX_PAGE_COUNT    8
#define SH1106_FRAMEBUFFER_SIZE  (SH1106_LCDWIDTH * SH1106_LCDHEIGHT / 8)
#define SH1106_COL_OFFSET        2

#define SH1106_SETCONTRAST       0x81
#define SH1106_DISPLAYALLON_RESUME 0xA4
#define SH1106_NORMALDISPLAY     0xA6
#define SH1106_DISPLAYOFF        0xAE
#define SH1106_DISPLAYON         0xAF
#define SH1106_SETDISPLAYOFFSET  0xD3
#define SH1106_SETCOMPINS        0xDA
#define SH1106_SETVCOMDETECT     0xDB
#define SH1106_SETDISPLAYCLOCKDIV 0xD5
#define SH1106_SETPRECHARGE      0xD9
#define SH1106_SETMULTIPLEX      0xA8
#define SH1106_SETLOWCOLUMN      0x00
#define SH1106_SETHIGHCOLUMN     0x10
#define SH1106_SETSTARTLINE      0x40
#define SH1106_MEMORYMODE        0x20
#define SH1106_SET_PAGE_ADDRESS  0xB0
#define SH1106_COMSCANDEC        0xC8
#define SH1106_SEGREMAP          0xA0
#define SH1106_CHARGEPUMP        0x8D

#ifdef __cplusplus

class sh1106 {
public:
	sh1106();
	bool init();
	void clear(bool update_hw = false);
	void display_on();
	void display_off();
	void display_buffer(const uint8_t *buffer);
	void drawPixel(int16_t x, int16_t y, uint8_t color);
	void setCursor(int16_t x, int16_t y);
	void setTextColor(uint8_t color);
	void print(const char *text);
	void update();
	void setBrightness(uint8_t brightness);

private:
	void command(uint8_t c);
	void data(uint8_t c);
	void drawChar(int16_t x, int16_t y, char ch, uint8_t color);

	uint8_t *framebuffer;
	int16_t cursor_x;
	int16_t cursor_y;
	uint8_t text_color;
};

extern "C" {
#endif

void bootloader_oled_show_dfu_mode(void);
void bootloader_oled_display_off(void);

#ifdef __cplusplus
}
#endif

#endif
