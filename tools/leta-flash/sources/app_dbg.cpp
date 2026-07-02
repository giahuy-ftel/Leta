#include "app_dbg.h"

#include <stdarg.h>
#include <stdio.h>

static enum def_debug_level current_debug_level = DEBUG_LEVEL_INFO;

static const char *debug_level_to_string(enum def_debug_level level)
{
	switch (level) {
	case DEBUG_LEVEL_ERROR:
		return "ERROR";
	case DEBUG_LEVEL_WARN:
		return "WARN";
	case DEBUG_LEVEL_INFO:
		return "INFO";
	case DEBUG_LEVEL_DEBUG:
		return "DEBUG";
	default:
		return "UNKNOWN";
	}
}

static const char *debug_level_to_color(enum def_debug_level level)
{
	switch (level) {
	case DEBUG_LEVEL_ERROR:
		return "\x1b[31m";
	case DEBUG_LEVEL_WARN:
		return "\x1b[33m";
	case DEBUG_LEVEL_INFO:
		return "\x1b[32m";
	case DEBUG_LEVEL_DEBUG:
		return "\x1b[36m";
	default:
		return "\x1b[0m";
	}
}

void app_dbg_set_level(enum def_debug_level level)
{
	current_debug_level = level;
}

void app_dbg_print(enum def_debug_level level, const char *format, ...)
{
	if (level <= DEBUG_LEVEL_NONE || level > DEBUG_LEVEL_DEBUG) {
		return;
	}

	if (current_debug_level < level) {
		return;
	}

	fputs(debug_level_to_color(level), stdout);
	printf("[%s] ", debug_level_to_string(level));
	fputs("\x1b[0m", stdout);

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	fputs("\x1b[0m", stdout);
}
