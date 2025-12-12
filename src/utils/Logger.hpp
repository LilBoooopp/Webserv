#pragma once
#include "iostream"
#include <cstdarg>
#include <cstdio>
#include <string>

enum LogChannel {
	LOG_NONE,
	LOG_INFO,
	LOG_SERVER,
	LOG_CONNECTION,
	LOG_ROUTER,
	LOG_CGI,
	LOG_REQUEST,
	LOG_RESPONSE,
	LOG_HEADER,
	LOG_BODY,
	LOG_ERROR,
	LOG_WARN,
	LOG_ALL,
};

const int loggerChannelsCount = LOG_ALL + 1;
const std::string LoggerLevels[LOG_ALL + 1] = {
    "NONE",	"INFO",	  "SERVER", "CONNECTION", "ROUTER",   "CGI", "REQUEST",
    "RESPONSE", "HEADER", "BODY",   "LOG_ERROR",  "LOG_WARN", "ALL"};

struct Logger {
	static bool channels[loggerChannelsCount];

	static void printChannels();
	static void setChannel(LogChannel lv) {
		if (lv == LOG_ALL || lv == LOG_NONE) {
			for (int i = 0; i < LOG_ALL; i++)
				channels[i] = (lv == LOG_ALL);
		} else if (lv >= LOG_NONE && lv < LOG_ALL)
			channels[lv] = !channels[lv];
	}
	static void unsetChannel(LogChannel lv) {
		if (lv == LOG_ALL) {
			for (int i = 0; i < LOG_ALL; i++)
				channels[i] = false;
		} else if (lv >= LOG_NONE && lv < LOG_ALL)
			channels[lv] = false;
	}
	static void setUntilChannel(LogChannel lv) {
		for (int i = 0; i < LOG_ALL; i++)
			channels[i] = (i <= lv);
	}

	static void error(const char *fmt, ...);
	static void info(const char *fmt, ...);
	static void simple(const char *fmt, ...);
	static void timer(const char *fmt, ...);
	static void server(const char *fmt, ...);
	static void connection(const char *fmt, ...);
	static void cgi(const char *fmt, ...);
	static void router(const char *fmt, ...);
	static void request(const char *fmt, ...);
	static void response(const char *fmt, ...);
	static void header(const char *fmt, ...);
	static void warn(const char *fmt, ...);
	static void action(const char *fmt, ...);
};

std::string toUpper(const std::string &s);