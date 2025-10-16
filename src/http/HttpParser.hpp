#pragma once
#include "HttpRequest.hpp"
#include <string>

class HttpParser {
public:
	static bool	parse(const std::string& buf, HttpRequest& req, size_t& endpos);
};
