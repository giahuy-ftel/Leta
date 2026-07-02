#include "system.h"

#include "boot_oled.h"
#include "button.h"
#include "io_cfg.h"
#include "main.h"
#include "stm32f1xx.h"

#define FLASH_KEY1 0x45670123UL
#define FLASH_KEY2 0xCDEF89ABUL

static volatile uint32_t system_ticks;
static button_t pa0_shutdown_button;
static uint32_t button_poll_ticks;
static bool buttons_ready;
static bool pa0_shutdown_event;

static void button_event_callback(void *ctx)
{
	button_t *button = (button_t *)ctx;

	if (button == &pa0_shutdown_button) {
		switch (button->state) {
		case BUTTON_SW_STATE_LONG_PRESSED:
			pa0_shutdown_event = true;
			break;

		default:
			break;
		}
	}
}

static void button_poll(void)
{
	button_timer_polling(&pa0_shutdown_button);
}

static void system_buttons_tick(void)
{
	if (!buttons_ready) {
		return;
	}

	button_poll_ticks++;
	if (button_poll_ticks >= SYSTEM_BUTTON_POLL_PERIOD_MS) {
		button_poll_ticks = 0;
		button_poll();
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

__attribute__((naked, noreturn)) static void app_start(uint32_t app_stack, uint32_t app_reset)
{
	(void)app_stack;
	(void)app_reset;

	__ASM volatile(
		"msr psp, r0\n"
		"movs r2, #0\n"
		"msr control, r2\n"
		"isb\n"
		"msr msp, r0\n"
		"bx r1\n"
	);
}

uint32_t system_millis(void)
{
	return system_ticks;
}

void system_tick(void)
{
	system_ticks++;
	system_buttons_tick();
}

uint32_t tusb_time_millis_api(void)
{
	return system_ticks;
}

void system_power_init(void)
{
	power_pin_init();
	power_pin_on();
}

void system_buttons_init(void)
{
	btn_ur_init();
	(void)button_init(&pa0_shutdown_button, SYSTEM_BUTTON_POLL_PERIOD_MS, BUTTON_HW_PRESSED_HIGH,
			btn_dr_init, btn_dr_read, button_event_callback);

	button_enable(&pa0_shutdown_button);
	button_poll_ticks = 0;
	pa0_shutdown_event = false;
	buttons_ready = true;
}

bool system_shutdown_requested(void)
{
	bool pressed = pa0_shutdown_event;
	pa0_shutdown_event = false;
	return pressed;
}

void system_power_off(void)
{
	bootloader_oled_display_off();
	power_pin_off();

	while (1) {
		__WFI();
	}
}

void system_flash_unlock(void)
{
	if (FLASH->CR & FLASH_CR_LOCK) {
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;
	}
}

void system_flash_lock(void)
{
	FLASH->CR |= FLASH_CR_LOCK;
}

bool system_flash_erase_page(uint32_t address)
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

bool system_flash_program_halfword(uint32_t address, uint16_t value)
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

bool system_app_is_valid(void)
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

void system_jump_to_app(void)
{
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
	app_start(app_stack, app_reset);
}
