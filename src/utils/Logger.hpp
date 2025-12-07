#pragma once
#include <cstdarg>
#include <cstdio>

enum LogLevel {
  LOG_NONE = -1,
  LOG_ERROR = 0,
  LOG_WARN = 1,
  LOG_INFO = 2,
  LOG_DEBUG = 3,
  LOG_ALL = 4
};

struct Logger {
  static LogLevel level;
  static void print_valid_levels();
  static void set_level(LogLevel lv) { level = lv; }
  static void error(const char *fmt, ...);
  static void warn(const char *fmt, ...);
  static void info(const char *fmt, ...);
  static void debug(const char *fmt, ...);
  static void simple(const char *fmt, ...);
  static void timer(const char *fmt, ...);
  static void action(const char *fmt, ...);
};
