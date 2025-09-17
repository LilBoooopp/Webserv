#pragma once
#include "IHandler.hpp"

struct	StaticHandler : IHandler {
	virtual void	handle(const HttpRequest& req, HttpResponse& res);
};
