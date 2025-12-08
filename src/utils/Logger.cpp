#include "Logger.hpp"
#include "Chrono.hpp"
#include "Colors.hpp"

bool Logger::channels[loggerChannelsCount] = {
    true,  // LOG_NONE (0)
    true,  // LOG_ERROR (1)
    true,  // LOG_INFO (2)
    true,  // LOG_SERVER (3)
    false, // LOG_CONN (4)
    true,  // LOG_CGI (5)
    true,  // LOG_REQUEST (6)
    true,  // LOG_RESPONSE (7)
    false, // LOG_HEADER (8)
    false   // LOG_BODY (9)
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
LOGGER_IMPL(info, "INFO", LOG_INFO, BLUE)
LOGGER_IMPL(server, "SERVER", LOG_SERVER, rgba(82, 96, 149, 1))
LOGGER_IMPL(connection, "Connection ", LOG_CONNECTION, rgba(111, 82, 149, 1))
LOGGER_IMPL(cgi, "CGI", LOG_CGI, rgba(190, 145, 103, 1))
LOGGER_IMPL(request, "REQUEST", LOG_REQUEST, rgba(76, 156, 116, 1))
LOGGER_IMPL(response, "RESPONSE", LOG_RESPONSE, rgba(166, 130, 193, 1))
LOGGER_IMPL(header, "  ", LOG_HEADER, rgba(215, 209, 147, 1))
LOGGER_IMPL(timer, "", LOG_ALL, TS)
LOGGER_IMPL(simple, NULL, LOG_ALL, NULL)

void Logger::printChannels() {
	for (int i = 1; i < LOG_ALL; ++i) {
		const std::string &name = LoggerLevels[i];
		int blockWidth = static_cast<int>(name.size()) + 3;

		std::ostringstream oss;
		oss << i;
		std::string idxStr = oss.str();

		int contentWidth = blockWidth - 1;
		int leftPad = (contentWidth - static_cast<int>(idxStr.size())) / 2;
		int rightPad = contentWidth - static_cast<int>(idxStr.size()) - leftPad;

		for (int s = 0; s < leftPad; ++s)
			std::fprintf(stderr, " ");

		std::fprintf(stderr, "%s%s%s", channels[i] ? GREEN : RED, idxStr.c_str(), TS);

		for (int s = 0; s < rightPad; ++s)
			std::fprintf(stderr, " ");

		std::fprintf(stderr, " ");
	}
	std::fprintf(stderr, "\n");

	for (int i = 1; i < LOG_ALL; ++i) {
		const std::string &name = LoggerLevels[i];
		std::fprintf(stderr, "%s[%s]%s ", channels[i] ? GREEN : RED, name.c_str(), TS);
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