#ifndef __ADAFRUIT_OLED_DRV_H
#define __ADAFRUIT_OLED_DRV_H

#include <stdlib.h>

#include "Adafruit_GFX.h"
#include "io_cfg.h"

#define OLED_COL_OFFSET 2

enum SH1106_CMD_SET {
	SH1106_SET_LOW_COLUMN = 0x00,
	SH1106_EXTERNAL_VCC = 0x01,
	SH1106_SWITCHCAP_VCC = 0x02,
	SH1106_SET_HIGH_COLUMN = 0x10,
	SH1106_MEMORY_MODE = 0x20,
	SH1106_COLUMN_ADDR = 0x21,
	SH1106_PAGE_ADDR = 0x22,
	SH1106_RIGHT_HORIZONTAL_SCROLL = 0x26,
	SH1106_LEFT_HORIZONTAL_SCROLL = 0x27,
	SH1106_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL = 0x29,
	SH1106_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL = 0x2A,
	SH1106_DEACTIVATE_SCROLL = 0x2E,
	SH1106_ACTIVATE_SCROLL = 0x2F,
	SH1106_SET_START_LINE = 0x40,
	SH1106_SET_CONTRAST = 0x81,
	SH1106_CHARGE_PUMP = 0x8D,
	SH1106_SEGREMAP = 0xA0,
	SH1106_SET_SEGMENT_REMAP = 0xA1,
	SH1106_SET_VERTICAL_SCROLL_AREA = 0xA3,
	SH1106_DISPLAY_ALL_ON_RESUME = 0xA4,
	SH1106_OUTPUT_FOLLOWS_RAM = 0xA4,
	SH1106_DISPLAY_ALL_ON = 0xA5,
	SH1106_NORMAL_DISPLAY = 0xA6,
	SH1106_INVERT_DISPLAY = 0xA7,
	SH1106_SET_MULTIPLEX = 0xA8,
	SH1106_DISPLAY_OFF = 0xAE,
	SH1106_DISPLAY_ON = 0xAF,
	SH1106_SET_PAGE_ADDRESS = 0xB0,
	SH1106_COM_SCAN_INC = 0xC0,
	SH1106_COM_SCAN_DEC = 0xC8,
	SH1106_SET_DISPLAY_OFFSET = 0xD3,
	SH1106_SET_DISPLAY_CLOCK_DIV = 0xD5,
	SH1106_SET_PRECHARGE = 0xD9,
	SH1106_SET_COM_PINS = 0xDA,
	SH1106_SET_VCOM_DETECT = 0xDB,
};

#define BLACK 0
#define WHITE 1

#define WIDTH 128
#define HEIGHT 64
#define FBSIZE 1024
#define MAXROW 8

class Adafruit_oled_drv : public Adafruit_GFX {
public:
	Adafruit_oled_drv();
	~Adafruit_oled_drv();

	virtual bool initialize();
	virtual void update();
	virtual void updateRow(int rowIndex);
	virtual void updateRow(int startRow, int endRow);
	virtual void drawPixel(int16_t x, int16_t y, uint16_t color);

	void clear(bool isUpdateHW = false);
	void display_on();
	void display_off();

	const unsigned char *getFrameBuffer() const;
	unsigned int getFrameBufferSize() const;

protected:
	void writeCommand(unsigned char cmd);
	void writeData(unsigned char data);

protected:
	unsigned char *m_pFramebuffer;
};

#endif
