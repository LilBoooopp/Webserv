#include "Logger.hpp"
#include "Chrono.hpp"
#include "Colors.hpp"

LogLevel Logger::level = LOG_ALL;

static void vlog(LogLevel want, const char *tag, const char *fmt, const char *clr, va_list ap) {
	if (want > Logger::level)
		return;
	if (tag)
		std::fprintf(stderr, "%s%s%s %-6s %s", rgb(163, 163, 163), getTimestamp().c_str(), clr, tag, TS);
	std::vfprintf(stderr, fmt, ap);
	std::fprintf(stderr, "%s\n", TS);
}

static void log_internal(LogLevel level, const char *tag, const char *color, const char *fmt,
			 va_list ap) {
	vlog(level, tag, fmt, color, ap);
}

#define LOGGER_IMPL(funcName, TAG, LEVEL, COLOR)                                                   \
	void Logger::funcName(const char *fmt, ...) {                                              \
		va_list ap;                                                                        \
		va_start(ap, fmt);                                                                 \
		log_internal(LEVEL, TAG, COLOR, fmt, ap);                                          \
		va_end(ap);                                                                        \
	}

LOGGER_IMPL(error, "ERROR", LOG_ERROR, RED)
LOGGER_IMPL(warn, "WARN", LOG_WARN, PURPLE)
LOGGER_IMPL(info, "INFO", LOG_INFO, rgb(82, 135, 149))
LOGGER_IMPL(debug, "DEBUG", LOG_DEBUG, YELLOW)
LOGGER_IMPL(timer, "", LOG_INFO, TS)
LOGGER_IMPL(simple, NULL, LOG_INFO, NULL)

void Logger::print_valid_levels() {
	if (level <= LOG_NONE)
		return;
	const char *levels[4] = {"ERROR", "WARN", "INFO", "DEBUG"};
	std::fprintf(stderr, "Logger %d - ", level);
	for (int i = 0; i < 4; i++)
		std::fprintf(stderr, "[%s%s%s] ", i < level ? GREEN : RED, levels[i], TS);
	std::fprintf(stderr, "\n");
}
