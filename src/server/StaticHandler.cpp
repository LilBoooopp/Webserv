#include "StaticHandler.hpp"
#include "../config/Config.hpp"
#include "../server/Server.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Mime.hpp"
#include "../utils/Path.hpp"
#include "../cgi/cgi.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static bool is_dir(const struct stat &st) { return (S_ISDIR(st.st_mode)); }
static bool is_reg(const struct stat &st) { return (S_ISREG(st.st_mode)); }

static bool read_file_small(const std::string &path, std::string &out) {
	int fd = ::open(path.c_str(), O_RDONLY);
	if (fd < 0)
		return (false);
	std::vector<char> buf(64 * 1024);
	out.clear();
	for (;;) {
		ssize_t r = ::read(fd, &buf[0], buf.size());
		if (r > 0)
			out.append(&buf[0], r);
		else
			break;
	}
	::close(fd);
	return (true);
}

void StaticHandler::handle(const HttpRequest &req, HttpResponse &res) {
	// We assume: method validated (ONLY GET for now)
	const ServerConfig &cfg = *cfg_;
	std::string path = safe_join_under_root(cfg.root, req.target);
	struct stat st;

	Logger::info("%s rooted %s%s%s -> %s%s", SERVER, GREY, req.target.c_str(), TS, GREY,
		     path.c_str());
	if (::stat(path.c_str(), &st) == 0) {
		if (is_dir(st)) {
			// try index file
			if (path.size() == 0 || path[path.size() - 1] != '/')
				path += '/';
			std::string idx = path + cfg.index;
			if (::stat(idx.c_str(), &st) == 0 && is_reg(st)) {
				std::string body;
				if (read_file_small(idx, body)) {
					res.setStatus(200, "OK");
					res.setBody(body);
					res.setContentType(mime_from_path(idx));
					return;
				}
			}
			res.setStatus(404, "Not Found");
			res.setBody("not found");
			res.setContentType("text/plain");
			return;
		} else if (is_reg(st)) {
			std::string body;
			if (read_file_small(path, body)) {
				res.setStatus(200, "OK");
				res.setBody(body);
				res.setContentType(mime_from_path(path));
				return;
			}
		}
	}
	res.setStatus(404, "Not Found");
	res.setBody("not found");
	res.setContentType("text/plain");
}
