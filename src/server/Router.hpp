#pragma once
#include "../config/Config.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/Path.hpp"
#include "IHandler.hpp"
#include <unistd.h>

class Router {
    private:
	const ServerConf &server_conf_;

	const LocationConf *matchLocation(const std::string &path) const;
	bool checkAllowedMethod(const HttpRequest &req, const LocationConf &loc,
				HttpResponse &res) const;

    public:
	Router(const ServerConf &conf);
	IHandler *route(Connection &c, const HttpRequest &req, HttpResponse &res);

	// Check if handler is a redirect and return target URL (empty string if not a redirect)
	static std::string getRedirectTarget(IHandler *handler);
	void redirectError(Connection &c);
};
