#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "stm32f1xx.h"
#include "system_stm32f10x.h"
#include "app_data.h"
#include "button.h"
#include "io_cfg.h"
#include "sh1106.h"
#include "tusb.h"

#define BOOTLOADER_START_ADDR 0x08000000UL
#define BOOTLOADER_END_ADDR   0x08006000UL
#define BOOT_STATE_ADDR       0x08006000UL
#define BOOT_STATE_END_ADDR   0x08007000UL
#define APP_START_ADDR        0x08007000UL
#define FLASH_END_ADDR        0x08020000UL
#define FLASH_PAGE_SIZE       1024UL

#define FLASH_KEY1            0x45670123UL
#define FLASH_KEY2            0xCDEF89ABUL

#define DFU_POWER_OFF_TIMEOUT_MS 120000UL
#define BUTTON_POLL_PERIOD_MS  10U

static volatile uint32_t system_ticks;
static uint32_t dfu_last_activity_ticks;
static uint32_t dfu_last_button_poll_ticks;
static uint32_t dfu_written_end_addr;
static bool dfu_update_marked;
static bool dfu_started;
static bool manifest_done;
static bool pa0_shutdown_event;
static button_t left_down_button;
static button_t pa0_shutdown_button;

static void button_event_callback(void *ctx);

typedef enum {
	BOOT_STATE_CHECK_BUTTONS,
	BOOT_STATE_READ_BOOT_STATE,
	BOOT_STATE_DFU,
	BOOT_STATE_JUMP_APP,
	BOOT_STATE_RECOVERY,
	BOOT_STATE_SHUTDOWN,
	BOOT_STATE_RESET,
} boot_state_t;

static void delay_ms(uint32_t ms)
{
	uint32_t start = system_ticks;
	while ((system_ticks - start) < ms) {
		__NOP();
	}
}

uint32_t tusb_time_millis_api(void)
{
	return system_ticks;
}

void SysTick_Handler(void)
{
	system_ticks++;
}

static void boot_power_init(void)
{
	power_pin_init();
	power_pin_on();
}

static void boot_buttons_init(void)
{
	(void)button_init(&left_down_button, BUTTON_POLL_PERIOD_MS, BUTTON_HW_PRESSED_LOW,
			btn_dl_init, btn_dl_read, button_event_callback);
	(void)button_init(&pa0_shutdown_button, BUTTON_POLL_PERIOD_MS, BUTTON_HW_PRESSED_HIGH,
			btn_dr_init, btn_dr_read, button_event_callback);

	button_enable(&left_down_button);
	button_enable(&pa0_shutdown_button);
}

static void button_event_callback(void *ctx)
{
	button_t *button = (button_t *)ctx;

	if (button == &pa0_shutdown_button && button->state == BUTTON_SW_STATE_LONG_PRESSED) {
		pa0_shutdown_event = true;
	}
}

static void button_poll(void)
{
	button_timer_polling(&left_down_button);
	button_timer_polling(&pa0_shutdown_button);
}

static bool left_down_pressed(void)
{
	return btn_dl_read() == 0;
}

static bool pa0_shutdown_pressed(void)
{
	bool pressed = pa0_shutdown_event;
	pa0_shutdown_event = false;
	return pressed;
}

static bool app_is_valid(void)
{
	uint32_t app_stack = *(volatile uint32_t *)APP_START_ADDR;
	uint32_t app_reset = *(volatile uint32_t *)(APP_START_ADDR + 4);

	if ((app_stack < SRAM_BASE) || (app_stack > (SRAM_BASE + 20UL * 1024UL))) {
		return false;
	}

	if ((app_reset < APP_START_ADDR) || (app_reset >= FLASH_END_ADDR) || ((app_reset & 1UL) == 0)) {
		return false;
	}

	return true;
}

static void jump_to_app(void)
{
	typedef void (*entry_t)(void);

	uint32_t app_stack = *(volatile uint32_t *)APP_START_ADDR;
	uint32_t app_reset = *(volatile uint32_t *)(APP_START_ADDR + 4);

	__disable_irq();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	for (uint32_t i = 0; i < 8; i++) {
		NVIC->ICER[i] = 0xFFFFFFFFUL;
		NVIC->ICPR[i] = 0xFFFFFFFFUL;
	}

	SCB->VTOR = APP_START_ADDR;
	__ASM volatile("msr msp, %0" : : "r"(app_stack));
	((entry_t)app_reset)();
}

static void usb_clock_init(void)
{
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

static void flash_unlock(void)
{
	if (FLASH->CR & FLASH_CR_LOCK) {
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;
	}
}

static bool flash_wait(void)
{
	while (FLASH->SR & FLASH_SR_BSY) {
	}

	if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
		FLASH->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
		return false;
	}

	FLASH->SR = FLASH_SR_EOP;
	return true;
}

static bool flash_erase_page(uint32_t address)
{
	if (!flash_wait()) {
		return false;
	}

	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = address;
	FLASH->CR |= FLASH_CR_STRT;

	bool ok = flash_wait();
	FLASH->CR &= ~FLASH_CR_PER;
	return ok;
}

static bool flash_program_halfword(uint32_t address, uint16_t value)
{
	if (!flash_wait()) {
		return false;
	}

	FLASH->CR |= FLASH_CR_PG;
	*(volatile uint16_t *)address = value;

	bool ok = flash_wait() && (*(volatile uint16_t *)address == value);
	FLASH->CR &= ~FLASH_CR_PG;
	return ok;
}

static void sys_boot_get(sys_boot_t *boot)
{
	if (boot != NULL) {
		memcpy(boot, (const void *)BOOT_STATE_ADDR, sizeof(*boot));
	}
}

static bool sys_boot_set(const sys_boot_t *boot)
{
	if (boot == NULL) {
		return false;
	}

	flash_unlock();

	if (!flash_erase_page(BOOT_STATE_ADDR)) {
		FLASH->CR |= FLASH_CR_LOCK;
		return false;
	}

	const uint8_t *data = (const uint8_t *)boot;
	for (uint32_t offset = 0; offset < sizeof(*boot); offset += 2) {
		uint16_t halfword = data[offset];
		if ((offset + 1U) < sizeof(*boot)) {
			halfword |= ((uint16_t)data[offset + 1U] << 8);
		} else {
			halfword |= 0xFF00U;
		}

		if (!flash_program_halfword(BOOT_STATE_ADDR + offset, halfword)) {
			FLASH->CR |= FLASH_CR_LOCK;
			return false;
		}
	}

	FLASH->CR |= FLASH_CR_LOCK;
	return true;
}

static bool sys_boot_erased_or_invalid(const sys_boot_t *boot)
{
	const uint32_t *words = (const uint32_t *)boot;

	if (boot == NULL) {
		return true;
	}

	for (uint32_t i = 0; i < (sizeof(*boot) / sizeof(uint32_t)); i++) {
		if (words[i] != 0xFFFFFFFFUL) {
			return false;
		}
	}

	return true;
}

static bool boot_app_metadata_ready(const sys_boot_t *boot)
{
	if (boot == NULL || sys_boot_erased_or_invalid(boot)) {
		return true;
	}

	return (boot->fw_app_cmd.cmd == SYS_BOOT_CMD_UPDATE_NONE) &&
			(boot->current_fw_app_header.psk == FIRMWARE_PSK);
}

static bool boot_direct_update_pending(const sys_boot_t *boot)
{
	if (boot == NULL || sys_boot_erased_or_invalid(boot)) {
		return false;
	}

	return (boot->fw_app_cmd.cmd == SYS_BOOT_CMD_UPDATE_REQ) &&
			(boot->fw_app_cmd.container == SYS_BOOT_CONTAINER_DIRECTLY);
}

static bool boot_external_or_unsupported_update_pending(const sys_boot_t *boot)
{
	if (boot == NULL || sys_boot_erased_or_invalid(boot)) {
		return false;
	}

	return (boot->fw_app_cmd.cmd == SYS_BOOT_CMD_UPDATE_REQ) &&
			(boot->fw_app_cmd.container != SYS_BOOT_CONTAINER_DIRECTLY);
}

static void boot_clear_return_mode(sys_boot_t *boot)
{
	if (boot == NULL || sys_boot_erased_or_invalid(boot)) {
		return;
	}

	boot->return_mode = SYS_BOOT_RETURN_NONE;
	(void)sys_boot_set(boot);
}

static boot_state_t boot_decide_state(sys_boot_t *boot)
{
	if (left_down_pressed()) {
		return BOOT_STATE_DFU;
	}

	if (boot != NULL && !sys_boot_erased_or_invalid(boot)) {
		if (boot->return_mode == SYS_BOOT_RETURN_BOOTLOADER) {
			boot_clear_return_mode(boot);
			return BOOT_STATE_DFU;
		}

		if (boot->return_mode == SYS_BOOT_RETURN_LAUNCHER) {
			boot_clear_return_mode(boot);
			return BOOT_STATE_DFU;
		}

		if (boot_direct_update_pending(boot)) {
			return BOOT_STATE_DFU;
		}

		if (boot_external_or_unsupported_update_pending(boot)) {
			return BOOT_STATE_RECOVERY;
		}
	}

	if (boot_app_metadata_ready(boot) && app_is_valid()) {
		return BOOT_STATE_JUMP_APP;
	}

	return BOOT_STATE_RECOVERY;
}

static uint32_t boot_app_image_length(void)
{
	if (dfu_written_end_addr <= APP_START_ADDR) {
		return 0;
	}

	return dfu_written_end_addr - APP_START_ADDR;
}

static void boot_mark_dfu_manifest_done(void)
{
	sys_boot_t boot;
	uint32_t image_length = boot_app_image_length();

	sys_boot_get(&boot);

	memset(&boot, 0xFF, sizeof(boot));
	boot.current_fw_app_header.psk = FIRMWARE_PSK;
	boot.current_fw_app_header.bin_len = image_length;
	boot.current_fw_app_header.checksum = 0;
	boot.fw_app_cmd.cmd = SYS_BOOT_CMD_UPDATE_NONE;
	boot.fw_app_cmd.container = SYS_BOOT_CONTAINER_DIRECTLY;
	boot.fw_app_cmd.io_driver = SYS_BOOT_IO_DRIVER_NONE;
	boot.fw_app_cmd.des_addr = APP_START_ADDR;
	boot.fw_app_cmd.src_addr = 0;
	boot.return_mode = SYS_BOOT_RETURN_NONE;

	(void)sys_boot_set(&boot);
}

static void boot_mark_dfu_update_pending(void)
{
	sys_boot_t boot;

	if (dfu_update_marked) {
		return;
	}

	sys_boot_get(&boot);
	if (sys_boot_erased_or_invalid(&boot)) {
		memset(&boot, 0xFF, sizeof(boot));
	}

	boot.fw_app_cmd.cmd = SYS_BOOT_CMD_UPDATE_REQ;
	boot.fw_app_cmd.container = SYS_BOOT_CONTAINER_DIRECTLY;
	boot.fw_app_cmd.io_driver = SYS_BOOT_IO_DRIVER_NONE;
	boot.fw_app_cmd.des_addr = APP_START_ADDR;
	boot.fw_app_cmd.src_addr = 0;
	boot.return_mode = SYS_BOOT_RETURN_NONE;

	(void)sys_boot_set(&boot);
	dfu_update_marked = true;
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

static void dfu_touch(void)
{
	dfu_last_activity_ticks = system_ticks;
}

static void dfu_power_off(void)
{
	bootloader_oled_display_off();
	power_pin_off();

	while (1) {
		__WFI();
	}
}

static void dfu_enter(void)
{
	bootloader_oled_show_dfu_mode();

	SysTick_Config(SystemCoreClock / 1000UL);
	dfu_touch();
	dfu_last_button_poll_ticks = system_ticks;
	usb_clock_init();

	tusb_rhport_init_t dev_init = {
		.role = TUSB_ROLE_DEVICE,
		.speed = TUSB_SPEED_AUTO
	};
	tusb_init(0, &dev_init);
	dfu_started = true;
}

static boot_state_t dfu_state_run(void)
{
	if (!dfu_started) {
		dfu_enter();
	}

	tud_task();

	if ((system_ticks - dfu_last_button_poll_ticks) >= BUTTON_POLL_PERIOD_MS) {
		dfu_last_button_poll_ticks = system_ticks;
		button_poll();
	}

	if (pa0_shutdown_pressed()) {
		return BOOT_STATE_SHUTDOWN;
	}

	if (manifest_done) {
		return BOOT_STATE_RESET;
	}

	if ((system_ticks - dfu_last_activity_ticks) >= DFU_POWER_OFF_TIMEOUT_MS) {
		return BOOT_STATE_SHUTDOWN;
	}

	return BOOT_STATE_DFU;
}

static boot_state_t boot_state_check_buttons(void)
{
	boot_power_init();
	boot_buttons_init();

	if (left_down_pressed()) {
		return BOOT_STATE_DFU;
	}

	return BOOT_STATE_READ_BOOT_STATE;
}

static boot_state_t boot_state_read_boot_state(void)
{
	sys_boot_t boot;

	sys_boot_get(&boot);
	return boot_decide_state(&boot);
}

static void boot_state_shutdown(void)
{
	dfu_power_off();
}

static void boot_state_reset(void)
{
	delay_ms(50);
	NVIC_SystemReset();
}

int main(void)
{
	boot_state_t state = BOOT_STATE_CHECK_BUTTONS;

	while (1) {
		switch (state) {
		case BOOT_STATE_CHECK_BUTTONS:
			state = boot_state_check_buttons();
			break;

		case BOOT_STATE_READ_BOOT_STATE:
			state = boot_state_read_boot_state();
			break;

		case BOOT_STATE_JUMP_APP:
			jump_to_app();
			state = BOOT_STATE_RECOVERY;
			break;

		case BOOT_STATE_RECOVERY:
			state = BOOT_STATE_DFU;
			break;

		case BOOT_STATE_DFU:
			state = dfu_state_run();
			break;

		case BOOT_STATE_SHUTDOWN:
			boot_state_shutdown();
			break;

		case BOOT_STATE_RESET:
			boot_state_reset();
			break;

		default:
			state = BOOT_STATE_RECOVERY;
			break;
		}
	}
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
	tud_int_handler(0);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	tud_int_handler(0);
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

	uint32_t address = APP_START_ADDR + ((uint32_t)block_num * CFG_TUD_DFU_XFER_BUFSIZE);
	uint8_t status = DFU_STATUS_OK;

	if (!dfu_range_valid(address, length)) {
		tud_dfu_finish_flashing(DFU_STATUS_ERR_ADDRESS);
		return;
	}

	if (length > 0) {
		boot_mark_dfu_update_pending();
	}

	flash_unlock();

	if ((address % FLASH_PAGE_SIZE) == 0) {
		if (!flash_erase_page(address)) {
			status = DFU_STATUS_ERR_ERASE;
		}
	}

	for (uint32_t offset = 0; (status == DFU_STATUS_OK) && (offset < length); offset += 2) {
		uint16_t halfword = data[offset];
		if ((offset + 1) < length) {
			halfword |= ((uint16_t)data[offset + 1] << 8);
		} else {
			halfword |= 0xFF00U;
		}

		if (!flash_program_halfword(address + offset, halfword)) {
			status = DFU_STATUS_ERR_PROG;
		}
	}

	if ((status == DFU_STATUS_OK) && (length > 0)) {
		uint32_t written_end = address + length;
		if (written_end > dfu_written_end_addr) {
			dfu_written_end_addr = written_end;
		}
	}

	FLASH->CR |= FLASH_CR_LOCK;
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
	boot_mark_dfu_manifest_done();
	manifest_done = true;
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
