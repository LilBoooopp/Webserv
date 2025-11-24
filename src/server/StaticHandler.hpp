#pragma once
#include "../config/Config.hpp"
#include "IHandler.hpp"

struct StaticHandler : IHandler {
	const ServerConfig *cfg_;
	explicit StaticHandler(const ServerConfig *cfg) : cfg_(cfg) {}
	virtual void handle(const HttpRequest &req, HttpResponse &res);
};
