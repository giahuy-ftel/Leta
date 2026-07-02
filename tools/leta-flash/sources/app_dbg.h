#ifndef __APP_DBG_H__
#define __APP_DBG_H__

enum def_debug_level {
	DEBUG_LEVEL_NONE = 0,
	DEBUG_LEVEL_ERROR,
	DEBUG_LEVEL_WARN,
	DEBUG_LEVEL_INFO,
	DEBUG_LEVEL_DEBUG
};

void app_dbg_set_level(enum def_debug_level level);
void app_dbg_print(enum def_debug_level level, const char *format, ...);

#define APP_DBG(level, format, ...) \
	do { \
		app_dbg_print(level, format, ##__VA_ARGS__); \
	} while (0)

#endif
