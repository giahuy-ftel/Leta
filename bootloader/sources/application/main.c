#include <stdbool.h>
#include <stddef.h>

#include "app_data.h"
#include "boot_dfu.h"
#include "boot_share.h"
#include "io_cfg.h"
#include "main.h"
#include "system.h"

typedef enum {
	BOOT_STATE_CHECK_BUTTONS,
	BOOT_STATE_READ_BOOT_STATE,
	BOOT_STATE_DFU,
	BOOT_STATE_JUMP_APP,
	BOOT_STATE_RECOVERY,
	BOOT_STATE_SHUTDOWN,
} boot_state_t;

static bool dfu_started;
static uint32_t dfu_manifest_done_ticks;

static bool boot_app_metadata_ready(const sys_boot_t *boot, bool boot_erased)
{
	if (boot == NULL || boot_erased) {
		return true;
	}

	return (boot->fw_app_cmd.cmd == SYS_BOOT_CMD_UPDATE_NONE) &&
			(boot->current_fw_app_header.psk == FIRMWARE_PSK);
}

static bool boot_direct_update_inprogress(const sys_boot_t *boot, bool boot_erased)
{
	if (boot == NULL || boot_erased) {
		return false;
	}

	return (boot->fw_app_cmd.cmd == SYS_BOOT_CMD_UPDATE_INPROGRESS) &&
			(boot->fw_app_cmd.container == SYS_BOOT_CONTAINER_DIRECTLY);
}

static void boot_clear_return_mode(sys_boot_t *boot, bool boot_erased)
{
	if (boot == NULL || boot_erased) {
		return;
	}

	boot->return_mode = SYS_BOOT_RETURN_NONE;
	(void)boot_share_set(boot);
}

static boot_state_t boot_decide_state(sys_boot_t *boot)
{
	bool boot_erased = boot_share_erased_or_invalid(boot);

	if (boot != NULL && !boot_erased) {
		if (boot->return_mode == SYS_BOOT_RETURN_BOOTLOADER) {
			boot_clear_return_mode(boot, boot_erased);
			return BOOT_STATE_DFU;
		}

		if (boot_direct_update_inprogress(boot, boot_erased)) {
			return BOOT_STATE_DFU;
		}
	}

	if (boot_app_metadata_ready(boot, boot_erased) && system_app_is_valid()) {
		return BOOT_STATE_JUMP_APP;
	}

	return BOOT_STATE_RECOVERY;
}

static boot_state_t dfu_state_run(void)
{
	if (!dfu_started) {
		boot_dfu_enter();
		dfu_started = true;
	}

	boot_dfu_task();

	if (system_shutdown_requested()) {
		return BOOT_STATE_SHUTDOWN;
	}

	if (boot_dfu_manifest_done()) {
		if (dfu_manifest_done_ticks == 0) {
			dfu_manifest_done_ticks = system_millis();
		}

		if ((system_millis() - dfu_manifest_done_ticks) >= BOOT_DFU_MANIFEST_GRACE_MS) {
			return BOOT_STATE_JUMP_APP;
		}
	}

	if (boot_dfu_timed_out()) {
		return BOOT_STATE_SHUTDOWN;
	}

	return BOOT_STATE_DFU;
}

static boot_state_t boot_state_check_buttons(void)
{
	system_power_init();
	btn_ur_init();

	if (btn_ur_read() == 0) {
		return BOOT_STATE_DFU;
	}

	return BOOT_STATE_READ_BOOT_STATE;
}

static boot_state_t boot_state_read_boot_state(void)
{
	sys_boot_t boot;

	boot_share_get(&boot);
	return boot_decide_state(&boot);
}

static void boot_state_shutdown(void)
{
	system_power_off();
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
			system_jump_to_app();
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

		default:
			state = BOOT_STATE_RECOVERY;
			break;
		}
	}
}
