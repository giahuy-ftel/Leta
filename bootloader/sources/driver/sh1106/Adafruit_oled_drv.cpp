#include "Adafruit_oled_drv.h"

#include <string.h>

static unsigned char frame_buffer[FBSIZE];

Adafruit_oled_drv::Adafruit_oled_drv() : Adafruit_GFX(WIDTH, HEIGHT) {
	m_pFramebuffer = 0;
}

Adafruit_oled_drv::~Adafruit_oled_drv() {
}

bool Adafruit_oled_drv::initialize() {
	oled_hw_init();
	oled_hw_reset();

	m_pFramebuffer = frame_buffer;
	if (m_pFramebuffer == 0) {
		return false;
	}

	memset(m_pFramebuffer, 0, FBSIZE);

	writeCommand(SH1106_DISPLAY_OFF);
	writeCommand(SH1106_SET_DISPLAY_CLOCK_DIV);
	writeCommand(0x80);
	writeCommand(SH1106_SET_MULTIPLEX);
	writeCommand(0x3F);
	writeCommand(SH1106_SET_DISPLAY_OFFSET);
	writeCommand(0x00);
	writeCommand(SH1106_SET_START_LINE | 0x0);
	writeCommand(SH1106_CHARGE_PUMP);
	writeCommand(0x14);
	writeCommand(SH1106_MEMORY_MODE);
	writeCommand(0x00);
	writeCommand(SH1106_SEGREMAP | 0x1);
	writeCommand(SH1106_COM_SCAN_DEC);
	writeCommand(SH1106_SET_COM_PINS);
	writeCommand(0x12);
	writeCommand(SH1106_SET_CONTRAST);
	writeCommand(0xCF);
	writeCommand(SH1106_SET_PRECHARGE);
	writeCommand(0xF1);
	writeCommand(SH1106_SET_VCOM_DETECT);
	writeCommand(0x40);
	writeCommand(SH1106_DISPLAY_ALL_ON_RESUME);
	writeCommand(SH1106_NORMAL_DISPLAY);
	writeCommand(SH1106_DISPLAY_ON);

	return true;
}

void Adafruit_oled_drv::clear(bool isUpdateHW) {
	memset(m_pFramebuffer, 0, FBSIZE);
	if (isUpdateHW) {
		update();
	}
}

void Adafruit_oled_drv::display_on() {
	writeCommand(SH1106_DISPLAY_ON);
}

void Adafruit_oled_drv::display_off() {
	writeCommand(SH1106_DISPLAY_OFF);
}

const unsigned char *Adafruit_oled_drv::getFrameBuffer() const {
	return m_pFramebuffer;
}

unsigned int Adafruit_oled_drv::getFrameBufferSize() const {
	return FBSIZE;
}

void Adafruit_oled_drv::writeCommand(unsigned char cmd) {
	oled_hw_write_byte(cmd, false);
}

void Adafruit_oled_drv::writeData(unsigned char data) {
	oled_hw_write_byte(data, true);
}

void Adafruit_oled_drv::drawPixel(int16_t x, int16_t y, uint16_t color) {
	unsigned char row;
	unsigned char offset;
	unsigned char preData;
	unsigned char val;
	int16_t index;

	if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()) || (m_pFramebuffer == 0)) {
		return;
	}

	row = y / 8;
	offset = y % 8;
	index = row * width() + x;
	preData = m_pFramebuffer[index];

	val = 1 << offset;
	if (color != 0) {
		m_pFramebuffer[index] = preData | val;
	} else {
		m_pFramebuffer[index] = preData & (~val);
	}
}

void Adafruit_oled_drv::update() {
	const uint8_t pageCount = HEIGHT >> 3;
	const uint8_t chunkCount = 8;
	const uint8_t chunkWidth = 132 >> 3;
	const uint8_t rowOffset = 0;
	const uint8_t colOffset = OLED_COL_OFFSET;
	int p = 0;

	writeCommand(SH1106_SET_LOW_COLUMN | (colOffset & 0x0F));
	writeCommand(SH1106_SET_HIGH_COLUMN | (colOffset >> 4));
	writeCommand(SH1106_SET_START_LINE | 0x0);

	for (uint8_t page = 0; page < pageCount; page++) {
		writeCommand(SH1106_SET_PAGE_ADDRESS + page + rowOffset);
		writeCommand(colOffset & 0x0F);
		writeCommand(0x10 | (colOffset >> 4));

		for (uint8_t chunk = 0; chunk < chunkCount; chunk++) {
			for (uint8_t col = 0; col < chunkWidth; col++, p++) {
				writeData(m_pFramebuffer[p]);
			}
		}
	}
}

void Adafruit_oled_drv::updateRow(int rowID) {
	if (rowID >= 0 && rowID < MAXROW && m_pFramebuffer) {
		const unsigned char colOffset = OLED_COL_OFFSET;
		const unsigned char lowerCol = colOffset & 0x0F;
		const unsigned char higherCol = 0x10 | (colOffset >> 4);

		writeCommand(SH1106_SET_PAGE_ADDRESS + rowID);
		writeCommand(higherCol);
		writeCommand(lowerCol);

		for (unsigned char x = 0; x < WIDTH; x++) {
			unsigned int index = rowID * WIDTH + x;
			writeData(m_pFramebuffer[index]);
		}
	}
}

void Adafruit_oled_drv::updateRow(int startID, int endID) {
	for (unsigned char y = startID; y < endID; y++) {
		updateRow(y);
	}
}
