#pragma once
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include <map>
#include <string>

struct HttpRequest {
	std::string method;
	std::string target;
	std::string version;
	std::map<std::string, std::string> headers;
};

inline static void logRequest(const HttpRequest &req) {
	if (!Logger::channels[LOG_REQUEST])
		return;
	const char *clr = rgba(122, 152, 126, 1);
	const char *methodClr = req.method == "GET"	 ? YELLOW
				: req.method == "POST"	 ? BLUE
				: req.method == "DELETE" ? rgb(110, 47, 193)
							 : GREEN;
	Logger::request("%sMethod %s%s%s%s, %starget%s %s, %sversion%s %s", GREY, methodClr, BLD,
			req.method.c_str(), TS, GREY, TS, req.target.c_str(), GREY, TS,
			req.version.c_str());
	if (!Logger::channels[LOG_HEADER])
		return;
	std::map<std::string, std::string>::const_iterator it;
	for (it = req.headers.begin(); it != req.headers.end(); it++)
		Logger::header("%s%-20s > %s", clr, it->first.c_str(), it->second.c_str());
}