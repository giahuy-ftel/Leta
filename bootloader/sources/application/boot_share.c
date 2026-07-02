#include "boot_share.h"

#include <stdint.h>
#include <string.h>

#include "main.h"
#include "system.h"

void boot_share_get(sys_boot_t *boot)
{
	if (boot != NULL) {
		memcpy(boot, (const void *)BOOT_STATE_ADDR, sizeof(*boot));
	}
}

bool boot_share_set(const sys_boot_t *boot)
{
	if (boot == NULL) {
		return false;
	}

	system_flash_unlock();

	if (!system_flash_erase_page(BOOT_STATE_ADDR)) {
		system_flash_lock();
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

		if (!system_flash_program_halfword(BOOT_STATE_ADDR + offset, halfword)) {
			system_flash_lock();
			return false;
		}
	}

	system_flash_lock();
	return true;
}

bool boot_share_erased_or_invalid(const sys_boot_t *boot)
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
