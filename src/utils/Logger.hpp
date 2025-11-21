#pragma once
#include <cstdio>
#include <cstdarg>

enum LogLevel { LOG_ERROR=0, LOG_WARN=1, LOG_INFO=2, LOG_DEBUG=3 };

struct Logger {
	static LogLevel	level;
	static void		set_level(LogLevel lv) { level = lv; }
	static void		error(const char* fmt, ...);
	static void		warn(const char* fmt, ...);
	static void		info(const char* fmt, ...);
	static void		debug(const char* fmt, ...);
};
