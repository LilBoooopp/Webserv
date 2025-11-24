#pragma once
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

struct	IHandler {
	virtual			~IHandler() {}
	virtual void	handle(const HttpRequest& req, HttpResponse& res) = 0;
};
