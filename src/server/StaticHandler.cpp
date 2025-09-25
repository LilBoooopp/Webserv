#include "StaticHandler.hpp"
#include "../config/Config.hpp"
#include "../utils/Mime.hpp"
#include "../utils/Path.hpp"
#include "../server/Server.hpp"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

static bool	is_dir(const struct stat& st) { return (S_ISDIR(st.st_mode)); }
static bool	is_reg(const struct stat& st) { return (S_ISREG(st.st_mode)); }

static bool	read_file_small(const std::string& path, std::string& out)
{
	int	fd = ::open(path.c_str(), O_RDONLY);
	if (fd < 0)
		return (false);
	std::vector<char> buf(64 * 1024);
	out.clear();
	for (;;)
	{
		ssize_t	r = ::read(fd, &buf[0], buf.size());
		if (r > 0)
			out.append(&buf[0], r);
		else
			break;
	}
	::close(fd);
	return (true);
}

void	StaticHandler::handle(const HttpRequest& req, HttpResponse& res) {
	// We assume: method validated (ONLY GET for now)
	const	ServerConfig&	cfg = *cfg_;
	std::string	path = safe_join_under_root(cfg.root, req.target);

	struct stat st;
	if (::stat(path.c_str(), &st) == 0)
	{
		if (is_dir(st))
		{
			// try index file
			if (path.size() == 0 || path[path.size() - 1] != '/')
				path += '/';
			std::string	idx = path + cfg.index;
			if (::stat(idx.c_str(), &st) == 0 && is_reg(st))
			{
				std::string	body;
				if (read_file_small(idx, body))
				{
					res.status = 200;
					res.reason = "OK";
					res.body = body;
					res.setContentType(mime_from_path(idx));
					res.close = true;
					return ;
				}
			}
			res.status = 404;
			res.reason = "Not Found";
			res.body = "not found";
			res.setContentType("text/plain");
			res.close = true;
			return	;
		}
		else if (is_reg(st))
		{
			std::string	body;
			if (read_file_small(path, body))
			{
				res.status = 200;
				res.reason = "OK";
				res.body = body;
				res.setContentType(mime_from_path(path));
				res.close = true;
				return	;
			}
		}
	}
	res.status = 404;
	res.reason = "Not Found";
	res.body = "not found";
	res.setContentType("text/plain");
	res.close = true;
}
