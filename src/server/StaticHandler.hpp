#pragma once
#include "../config/Config.hpp"
#include "IHandler.hpp"
#include "../config/Config.hpp"

struct StaticHandler : IHandler {
	const std::vector<ServerConf> *cfg_;
	explicit StaticHandler(const std::vector<ServerConf> *cfg) : cfg_(cfg) {}
	virtual void handle(const HttpRequest &req, HttpResponse &res);
};
