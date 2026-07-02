#include "usb_dfu.h"

#include <stdlib.h>
#include <string>

#include "app_dbg.h"

#define LETA_USB_VID "0309"
#define LETA_USB_PID "2001"

static std::string shell_quote(const std::string &value)
{
	std::string quoted = "'";

	for (size_t i = 0; i < value.size(); i++) {
		if (value[i] == '\'') {
			quoted += "'\\''";
		} else {
			quoted += value[i];
		}
	}

	quoted += "'";
	return quoted;
}

bool usb_dfu_flash(const std::string &dfu_path)
{
	std::string command = "dfu-util -d ";
	command += LETA_USB_VID;
	command += ":";
	command += LETA_USB_PID;
	command += " -a 0 -D ";
	command += shell_quote(dfu_path);

	APP_DBG(DEBUG_LEVEL_INFO, "%s\n", command.c_str());

	int result = system(command.c_str());
	if (result != 0) {
		APP_DBG(DEBUG_LEVEL_ERROR, "dfu-util failed: exit_status=%d\n", result);
		return false;
	}

	return true;
}
