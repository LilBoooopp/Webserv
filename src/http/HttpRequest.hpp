#pragma once
#include <map>
#include <string>

struct HttpRequest {
	std::string method;
	std::string target;
	std::string version;
	std::map<std::string, std::string> headers;
};
