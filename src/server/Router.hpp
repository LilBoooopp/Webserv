#pragma once
#include "../config/Config.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/Connection.hpp"
#include "../utils/Path.hpp"
#include <unistd.h>
#include <dirent.h>

class Router {
    public:
	Router() {};

	// Check if handler is a redirect and return target URL (empty string if not a redirect)
	static void loadErrorPage(Connection &c);
	static bool checkAllowedMethod(Connection &c, const LocationConf &loc);
	static std::string resolvePath(const ServerConf &conf, const LocationConf *loc,
				       const std::string &target);
	static const LocationConf *matchLocation(const ServerConf &conf, const std::string &path);
	static bool route(Connection &c);
	static void handle(Connection &c);
	static void finalizeResponse(Connection &c);
};
