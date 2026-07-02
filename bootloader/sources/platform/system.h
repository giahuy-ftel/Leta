#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

#define SYSTEM_BUTTON_POLL_PERIOD_MS   10U

uint32_t system_millis(void);
void system_tick(void);
void system_power_init(void);
void system_buttons_init(void);
bool system_shutdown_requested(void);
void system_power_off(void);
void system_flash_unlock(void);
void system_flash_lock(void);
bool system_flash_erase_page(uint32_t address);
bool system_flash_program_halfword(uint32_t address, uint16_t value);
bool system_app_is_valid(void);
void system_jump_to_app(void);

#endif
