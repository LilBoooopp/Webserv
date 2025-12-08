#include "Logger.hpp"
#include "Chrono.hpp"
#include "Colors.hpp"

bool Logger::channels[loggerChannelsCount] = {
    true, // LOG_NONE (0)
    true, // LOG_ERROR (1)
    true, // LOG_WARN (2)
    true, // LOG_INFO (3)
    true, // LOG_DEBUG (4)
    true, // LOG_SERVER (5)
    false, // LOG_CONN (6)
    true, // LOG_CGI (7)
    true, // LOG_REQUEST (8)
    true, // LOG_RESPONSE (9)
    false // LOG_HEADER (10)
};

static void vlog(LogChannel want, const char *tag, const char *fmt, const char *clr, va_list ap) {
	if (want >= LOG_NONE && want < LOG_ALL && !Logger::channels[want])
		return;
	if (tag) {
		if (want == LOG_HEADER || want == LOG_CONNECTION)
			std::fprintf(stderr, "%s%s%s", clr, tag, TS);
		else
			std::fprintf(stderr, "%s%s%s %-6s %s", rgb(163, 163, 163),
				     getTimestamp().c_str(), clr, tag, TS);
	}

	std::vfprintf(stderr, fmt, ap);
	std::fprintf(stderr, "%s\n", TS);
}

static void log_internal(LogChannel level, const char *tag, const char *color, const char *fmt,
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
LOGGER_IMPL(server, "SERVER", LOG_SERVER, rgba(82, 96, 149, 1))
LOGGER_IMPL(connection, "Connection ", LOG_CONNECTION, rgba(82, 114, 149, 1))
LOGGER_IMPL(cgi, "CGI", LOG_CGI, rgba(149, 82, 130, 1))
LOGGER_IMPL(request, "HTTP_REQ.", LOG_REQUEST, rgba(76, 156, 116, 1))
LOGGER_IMPL(response, "HTTP_RES.", LOG_RESPONSE, rgba(143, 156, 76, 1))
LOGGER_IMPL(header, "  ", LOG_HEADER, rgba(175, 188, 196, 1))
LOGGER_IMPL(timer, "", LOG_ALL, TS)
LOGGER_IMPL(simple, NULL, LOG_ALL, NULL)

void Logger::printChannels() {
	for (int i = 1; i < LOG_ALL; i++) {
		std::string shortName = LoggerLevels[i].substr(0, 6);
		std::fprintf(stderr, "[%s%d %s%s] ", channels[i] ? GREEN : RED, i,
			     shortName.c_str(), TS);
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