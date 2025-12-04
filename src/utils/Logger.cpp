#include "Logger.hpp"
#include "Chrono.hpp"
#include "Colors.hpp"

LogLevel Logger::level = LOG_ALL;
LogLevel Logger::exclusive = LOG_NONE;

static void vlog(LogLevel want, const char *tag, const char *fmt, const char *clr, va_list ap) {
	if (Logger::exclusive != LOG_NONE) {
		if (Logger::exclusive != LOG_ALL && want != Logger::exclusive)
			return;
	} else if (want > Logger::level)
		return;
	if (tag)
		std::fprintf(stderr, "%s%s%s %-6s %s", rgb(163, 163, 163), getTimestamp().c_str(),
			     clr, tag, TS);
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
LOGGER_IMPL(auth, "AUTH", LOG_AUTH, rgba(125, 82, 149, 1))
LOGGER_IMPL(debug, "DEBUG", LOG_DEBUG, YELLOW)
LOGGER_IMPL(timer, "", LOG_INFO, TS)
LOGGER_IMPL(simple, NULL, LOG_INFO, NULL)

void Logger::print_valid_levels() {
	std::fprintf(stderr, "Level=%s Exclusive=%s - ", LoggerLevels[level].c_str(),
		     LoggerLevels[exclusive].c_str());
	for (int i = 1; i < loggerLevelsCount; i++) {
		const char *clr = exclusive == i ? YELLOW : exclusive == LOG_ALL || (i <= level && (exclusive == LOG_NONE)) ? GREEN : RED;
		std::fprintf(stderr, "[%s%s%s] ", clr, LoggerLevels[i].c_str(), TS);
	}
	std::fprintf(stderr, "\n");
}

std::string toUpper(const std::string &s) {
	std::string S = s;
	for (size_t i = 0; i < s.size(); ++i) {
		S[i] = std::toupper(static_cast<unsigned char>(s[i]));
	}
	return S;
}