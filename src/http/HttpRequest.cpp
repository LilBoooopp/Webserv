#include "HttpRequest.hpp"

void HttpRequest::log() const {
	if (!Logger::channels[LOG_REQUEST])
		return;
	const char *clr = rgba(122, 152, 126, 1);
	const char *methodClr = method == "GET"	     ? YELLOW
				: method == "POST"   ? BLUE
				: method == "DELETE" ? rgb(110, 47, 193)
						     : GREEN;
	Logger::request("%sMethod %s%s%s%s, %starget %s%s, %sversion%s %s", GREY, methodClr, BLD,
			method.c_str(), TS, GREY, URLCLR, target.c_str(), GREY, TS,
			version.c_str());
	if (!Logger::channels[LOG_HEADER])
		return;
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); it++)
		Logger::header("%s%-20s > %s", clr, it->first.c_str(), it->second.c_str());
}

bool HttpRequest::hasHeader(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = headers.find(key);
	return (it != headers.end());
}

bool HttpRequest::eraseHeader(const std::string &key) { return headers.erase(key) > 0; }

std::string HttpRequest::getHeader(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = headers.find(key);
	if (it != headers.end())
		return it->second;
	return std::string();
}

void HttpRequest::removeBoundary(std::string &body, const std::string &boundary) {
	std::string fullBoundary = "--" + boundary;
	size_t pos = 0;

	while ((pos = body.find(fullBoundary, pos)) != std::string::npos) {
		size_t eraseLen = fullBoundary.length();
		if (pos + eraseLen + 1 < body.length() && body[pos + eraseLen] == '\r' &&
		    body[pos + eraseLen + 1] == '\n')
			eraseLen += 2;
		body.erase(pos, eraseLen);
	}
}