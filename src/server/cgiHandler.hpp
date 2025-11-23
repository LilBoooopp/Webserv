#pragma once
#include "../http/HttpResponse.hpp"
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "../config/Config.hpp"
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

struct cgiHandler {
	static std::string runCgi(const std::string &path, const std::string &queryString,
			   HttpResponse &res, const ServerConfig &cfg);
	static std::string extractArguments(std::string &pathToCgi);
};
