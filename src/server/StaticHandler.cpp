#include "StaticHandler.hpp"

void	StaticHandler::handle(const HttpRequest& req, HttpResponse& res) {
	(void)req;
	res.status = 200;
	res.reason = "OK";
	res.contentType = "text/plain";
	res.body = "hello";
	res.close = true;
}
