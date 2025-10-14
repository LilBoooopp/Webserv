#include "Logger.hpp"

LogLevel Logger::level = LOG_INFO;

static void	vlog(LogLevel want, const char* tag, const char* fmt, va_list ap)
{
	if (want > Logger::level)
		return ;
	std::fprintf(stderr, "[%s] ", tag);
	std::vfprintf(stderr, fmt, ap);
	std::fprintf(stderr, "\n");
}

void Logger::error(const char* fmt, ...) { va_list	ap; va_start(ap, fmt); vlog(LOG_ERROR, "ERROR", fmt, ap); va_end(ap); }
void Logger::warn(const char* fmt, ...) { va_list	ap; va_start(ap, fmt); vlog(LOG_ERROR, "WARN", fmt, ap); va_end(ap); }
void Logger::info(const char* fmt, ...) { va_list	ap; va_start(ap, fmt); vlog(LOG_ERROR, "INFO", fmt, ap); va_end(ap); }
void Logger::debug(const char* fmt, ...) { va_list	ap; va_start(ap, fmt); vlog(LOG_ERROR, "DEBUG", fmt, ap); va_end(ap); }
