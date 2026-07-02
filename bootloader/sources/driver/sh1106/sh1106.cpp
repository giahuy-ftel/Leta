#include "sh1106.h"

#include "io_cfg.h"

#include <string.h>

static uint8_t sh1106_framebuffer[SH1106_FRAMEBUFFER_SIZE];

static const uint8_t font_blank[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t font_B[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
static const uint8_t font_D[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
static const uint8_t font_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
static const uint8_t font_M[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
static const uint8_t font_S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
static const uint8_t font_U[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
static const uint8_t font_d[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
static const uint8_t font_e[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
static const uint8_t font_o[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};

static const uint8_t *font_for_char(char ch)
{
	switch (ch) {
	case 'B':
		return font_B;
	case 'D':
		return font_D;
	case 'F':
		return font_F;
	case 'M':
		return font_M;
	case 'S':
		return font_S;
	case 'U':
		return font_U;
	case 'd':
		return font_d;
	case 'e':
		return font_e;
	case 'o':
		return font_o;
	default:
		return font_blank;
	}
}

sh1106::sh1106()
{
}

void sh1106::command(uint8_t c)
{
	oled_hw_write_byte(c, false);
}

void sh1106::data(uint8_t c)
{
	oled_hw_write_byte(c, true);
}

bool sh1106::init()
{
	framebuffer = sh1106_framebuffer;
	cursor_x = 0;
	cursor_y = 0;
	text_color = WHITE;

	oled_hw_init();
	oled_hw_reset();

	memset(framebuffer, 0, SH1106_FRAMEBUFFER_SIZE);

	command(SH1106_DISPLAYOFF);
	command(SH1106_SETDISPLAYCLOCKDIV);
	command(0xF0);
	command(SH1106_SETMULTIPLEX);
	command(0x3F);
	command(SH1106_DISPLAYALLON_RESUME);
	command(SH1106_SETDISPLAYOFFSET);
	command(0x00);
	command(SH1106_SETSTARTLINE);
	command(SH1106_CHARGEPUMP);
	command(0x14);
	command(SH1106_MEMORYMODE);
	command(0x00);
	command(SH1106_SET_PAGE_ADDRESS);
	command(SH1106_COMSCANDEC);
	command(SH1106_SETLOWCOLUMN);
	command(SH1106_SETHIGHCOLUMN);
	command(SH1106_SETCOMPINS);
	command(0x12);
	command(SH1106_SETCONTRAST);
	command(0xCF);
	command(SH1106_SEGREMAP | 0x01);
	command(SH1106_SETPRECHARGE);
	command(0xF1);
	command(SH1106_SETVCOMDETECT);
	command(0x40);
	command(SH1106_DISPLAYALLON_RESUME);
	command(SH1106_NORMALDISPLAY);
	clear(true);
	command(SH1106_DISPLAYON);

	return true;
}

void sh1106::clear(bool update_hw)
{
	memset(framebuffer, 0, SH1106_FRAMEBUFFER_SIZE);
	cursor_x = 0;
	cursor_y = 0;

	if (update_hw) {
		update();
	}
}

void sh1106::display_on()
{
	command(SH1106_DISPLAYON);
}

void sh1106::display_off()
{
	command(SH1106_DISPLAYOFF);
}

void sh1106::display_buffer(const uint8_t *buffer)
{
	if (buffer == 0) {
		return;
	}

	for (uint8_t page = 0; page < SH1106_MAX_PAGE_COUNT; page++) {
		command(SH1106_SET_PAGE_ADDRESS + page);
		command(SH1106_SETLOWCOLUMN | (SH1106_COL_OFFSET & 0x0F));
		command(SH1106_SETHIGHCOLUMN | (SH1106_COL_OFFSET >> 4));

		for (uint8_t col = 0; col < SH1106_LCDWIDTH; col++) {
			data(buffer[(page * SH1106_LCDWIDTH) + col]);
		}
	}
}

void sh1106::update()
{
	display_buffer(framebuffer);
}

void sh1106::drawPixel(int16_t x, int16_t y, uint8_t color)
{
	if ((x < 0) || (x >= SH1106_LCDWIDTH) || (y < 0) || (y >= SH1106_LCDHEIGHT)) {
		return;
	}

	uint16_t index = ((uint16_t)(y >> 3) * SH1106_LCDWIDTH) + (uint16_t)x;
	uint8_t mask = (uint8_t)(1U << (y & 0x07));

	if (color == WHITE) {
		framebuffer[index] |= mask;
	} else if (color == BLACK) {
		framebuffer[index] &= (uint8_t)~mask;
	} else {
		framebuffer[index] ^= mask;
	}
}

void sh1106::setCursor(int16_t x, int16_t y)
{
	cursor_x = x;
	cursor_y = y;
}

void sh1106::setTextColor(uint8_t color)
{
	text_color = color;
}

void sh1106::drawChar(int16_t x, int16_t y, char ch, uint8_t color)
{
	const uint8_t *bitmap = font_for_char(ch);

	for (uint8_t i = 0; i < 5; i++) {
		uint8_t line = bitmap[i];
		for (uint8_t j = 0; j < 8; j++) {
			if ((line & 0x01U) != 0) {
				drawPixel(x + i, y + j, color);
			}
			line >>= 1;
		}
	}
}

void sh1106::print(const char *text)
{
	if (text == 0) {
		return;
	}

	while (*text != '\0') {
		if (*text == '\n') {
			cursor_y += 8;
			cursor_x = 0;
		} else if (*text != '\r') {
			drawChar(cursor_x, cursor_y, *text, text_color);
			cursor_x += 6;
		}
		text++;
	}
}

void sh1106::setBrightness(uint8_t brightness)
{
	command(SH1106_SETCONTRAST);
	command(brightness);
}

static sh1106 bootloader_oled;
static bool bootloader_oled_ready;

extern "C" void bootloader_oled_show_dfu_mode(void)
{
	if (!bootloader_oled_ready) {
		bootloader_oled_ready = bootloader_oled.init();
	}

	bootloader_oled.clear(false);
	bootloader_oled.setCursor(28, 24);
	bootloader_oled.setTextColor(WHITE);
	bootloader_oled.print("USB DFU Mode");
	bootloader_oled.update();
	bootloader_oled.display_on();
}

extern "C" void bootloader_oled_display_off(void)
{
	if (bootloader_oled_ready) {
		bootloader_oled.display_off();
	}
}
