#include "Server.hpp"

static void handleGet(Connection &c) {
	const std::string kStreamHeader = "X-Stream-File";
	if (!c.res.hasHeader(kStreamHeader))
		return;

	std::string file_path = c.res.getHeader(kStreamHeader);
	//	Try to open the file now, in non-streaming, blocking mode.
	//	We will only read it in small chunks later form handleWritable.
	int ffd = ::open(file_path.c_str(), O_RDONLY);
	if (ffd >= 0) {
		c.file_fd = ffd;
		c.streaming_file = true;

		//	Initialize remaining bytes from Content-Length
		std::string cl = c.res.getHeader("Content-Length");
		off_t remaining = 0;
		if (!cl.empty()) {
			std::istringstream iss(cl);
			iss >> remaining;
		}
		c.file_remaining = remaining;

		//	Remove the internal header so the client doesn't see it.
		c.res.eraseHeader(kStreamHeader);
	} else {
		//	If open fails, fall back to a 404
		c.res.setStatusFromCode(404);

		// ensure no streaming
		c.streaming_file = false;
		c.file_fd = -1;
		c.file_remaining = 0;
	}
}

static void handlePost(Connection &c) {
	const std::string &target_file = Router::resolvePath(c.cfg, c.loc, c.req.target);

	// Open file for writing (will create or overwrite)
	std::ofstream file(target_file.c_str(), std::ios::binary | std::ios::trunc);
	if (file.fail()) {
		c.res.setStatusFromCode(500);
		return;
	}

	std::string contentType = c.req.getHeader("Content-Type");
	if (contentType.find("multipart/form-data") != std::string::npos) {
		size_t pos = contentType.find("boundary=");
		if (pos != std::string::npos) {
			std::string body = c.body;
			std::string boundary = contentType.substr(pos + 9);
			c.req.removeBoundary(body, boundary);
			file.write(body.c_str(), body.length());
			return;
		}
	}
	file.write(c.body.c_str(), c.body.length());
	return;
}

static void handleDelete(Connection &c) {
	if (!c.loc) {
		c.res.setStatusFromCode(403);
		c.res.setHeader("Allow", "GET, HEAD");
		return;
	}
	if (!Router::checkAllowedMethod(c, *c.loc))
		return;
	const std::string &target_file = Router::resolvePath(c.cfg, c.loc, c.req.target);
	Logger::response("%srouted \'%s%s%s\' -> %s%s%s", GREY, URLCLR, c.req.target.c_str(), TS,
			 URLCLR, target_file.c_str(), TS);
	if (!file_exists(target_file))
		c.res.setStatusFromCode(404);
	else if (remove(target_file.c_str()) != 0)
		c.res.setStatusFromCode(500);
	else
		Logger::response("file at path \'%s%s%s\' deleted.", URLCLR, target_file.c_str(),
				 TS);
}

// Build and serialize a response once the request/body are ready
void Server::prepareResponse(int fd, Connection &c) {
	if (c.res.getStatus() >= 400) {
		Router::finalizeResponse(c);
		enableWrite(fd);
		return;
	}

	c.req.log();
	c.res.setVersion(c.req.version);

	if (is_cgi(c.req.target, c.cfg)) {
		if (!cgiHandler_.runCgi(c, fd)) {
			Router::finalizeResponse(c);
			enableWrite(fd);
		} else {
			c.state = WAITING_CGI;
		}
		return;
	}
	if (!Router::route(c) && (c.req.method == "GET" || c.req.method == "HEAD"))
		Router::handle(c);
	if (c.req.method == "GET")
		handleGet(c);
	else if (c.req.method == "POST")
		handlePost(c);
	else if (c.req.method == "DELETE")
		handleDelete(c);
	else if (c.req.method != "HEAD")
		c.res.setStatusFromCode(405);
	Router::finalizeResponse(c);
	enableWrite(fd);
}
