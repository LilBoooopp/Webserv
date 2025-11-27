#pragma once
#include "../config/Config.hpp"
#include "IHandler.hpp"
#include "../config/Config.hpp"

struct StaticHandler : IHandler {
	const Conf *cfg_;
	explicit StaticHandler(const Conf *cfg) : cfg_(cfg) {}
	virtual void handle(const HttpRequest &req, HttpResponse &res);
};
