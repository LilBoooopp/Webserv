#pragma once
#include <cstdarg>
#include <cstdio>
#include <string>

enum LogLevel {
	LOG_NONE = 0,
	LOG_ERROR = 1,
	LOG_WARN = 2,
	LOG_INFO = 3,
	LOG_DEBUG = 4,
	LOG_AUTH = 5,
	LOG_ALL = 6
};
const int loggerLevelsCount = LOG_ALL + 1;
const std::string LoggerLevels[loggerLevelsCount] = {"NONE",  "ERROR", "WARN", "INFO",
						     "DEBUG", "AUTH",  "ALL"};

struct Logger {
	static LogLevel level;
	static LogLevel exclusive;

	static void print_valid_levels();
	static void set_level(LogLevel lv) {
		level = (lv <= LOG_NONE ? LOG_NONE : lv >= loggerLevelsCount ? LOG_ALL : lv);
	}
	static void set_exclusiveLevel(LogLevel lv) {
		exclusive = (lv <= LOG_NONE ? LOG_NONE : lv >= loggerLevelsCount ? LOG_ALL : lv);
	}

	static void error(const char *fmt, ...);
	static void warn(const char *fmt, ...);
	static void info(const char *fmt, ...);
	static void debug(const char *fmt, ...);
	static void simple(const char *fmt, ...);
	static void timer(const char *fmt, ...);
	static void auth(const char *fmt, ...);
	static void action(const char *fmt, ...);
};

std::string toUpper(const std::string &s);