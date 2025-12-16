#include "Router.hpp"
#include "../utils/Logger.hpp"
#include "IHandler.hpp"
#include "StaticHandler.hpp"

/**
 * @class ErrorHandler
 * @brief Simple handler for setting error status
 *
 */
struct ErrorHandler : IHandler {
	int status_code;
	ErrorHandler(int code) : status_code(code) {}
	virtual void handle(Connection &c, const HttpRequest &req, HttpResponse &res) {
		(void)c;
		(void)req;
		res.setStatusFromCode(status_code);
		res.ensureDefaultBodyIfEmpty();
	}
};

struct RedirectHandler : IHandler {
	int status_code;
	std::string target_url;
	RedirectHandler(int code, const std::string &url) : status_code(code), target_url(url) {}
	virtual void handle(Connection &c, const HttpRequest &req, HttpResponse &res) {
		(void)c;
		(void)req;
		res.setStatusFromCode(status_code);
		res.setHeader("Location", target_url);
	}
};

std::string Router::getRedirectTarget(IHandler *handler) {
	RedirectHandler *redirect = dynamic_cast<RedirectHandler *>(handler);
	if (redirect)
		return redirect->target_url;
	return "";
}

Router::Router(const ServerConf &conf) : server_conf_(conf) {}

/**
 * @brief Finds the best matching location block using the longest prefix match
 *
 * @param path The request target path
 * @return const LocationConf* Pointer to the best matching location, or NULL if
 * none
 */
const LocationConf *Router::matchLocation(const std::string &path) const {
	const LocationConf *best_match = NULL;
	size_t longest_match_len = 0;

	for (size_t i = 0; i < server_conf_.locations.size(); ++i) {
		const LocationConf &loc = server_conf_.locations[i];
		const std::string &loc_path = loc.path;

		if (path.rfind(loc_path, 0) == 0) {
			bool is_valid_match =
			    (loc_path.size() == path.size() ||
			     (loc_path.size() < path.size() && path[loc_path.size()] == '/'));

			if (is_valid_match && loc_path.size() > longest_match_len) {
				longest_match_len = loc_path.size();
				best_match = &loc;
			}
		}
	}
	return (best_match);
}

/**
 * @brief Checks if the request method is allowed for location
 *
 * @param req
 * @param loc
 * @param res
 * @return true if allowed, flase otherwise
 */
bool Router::checkAllowedMethod(const HttpRequest &req, const LocationConf &loc,
				HttpResponse &res) const {
	if (loc.methods.empty())
		return (true);

	const std::string &method = req.method;
	bool allowed = false;
	for (size_t i = 0; i < loc.methods.size(); ++i) {
		// HEAD should be treated as GET for permission purposes
		if (method == "HEAD" && loc.methods[i] == "GET") {
			allowed = true;
			break;
		}
		if (loc.methods[i] == method) {
			allowed = true;
			break;
		}
	}

	if (!allowed) {
		res.setStatusFromCode(405);
		std::string allow_header;
		for (size_t i = 0; i < loc.methods.size(); ++i) {
			if (i > 0)
				allow_header += ", ";
			allow_header += loc.methods[i];
		}
		res.setHeader("Allow", allow_header);
		return (false);
	}
	return (true);
}

/**
 * @brief The main routing
 *
 * @param c
 * @param req
 * @param res
 * @return IHandler* The handler to process the request.
 */
IHandler *Router::route(Connection &c, const HttpRequest &req, HttpResponse &res) {
	(void)c;
	const LocationConf *matched_loc = matchLocation(req.target);

	if (matched_loc) {
		const LocationConf &loc = *matched_loc;
		Logger::router("Matched location: %s", loc.path.c_str());

		if (!checkAllowedMethod(req, loc, res)) {
			Logger::router("unvalid method for \'%s\' location", loc.path.c_str());
			return (new ErrorHandler(405));
		}

		if (loc.redirect_enabled) {
			Logger::router("Redirecting %s to %s (%d).", req.target.c_str(),
				       loc.redirect_target.c_str(), loc.redirect_status);
			return (new RedirectHandler(loc.redirect_status, loc.redirect_target));
		}

		Logger::router("Using StaticHandler with location %s", loc.path.c_str());
		return (new StaticHandler(server_conf_, matched_loc));
	}

	Logger::router("No location matched %s. Using server defaults.", req.target.c_str());
	return (new StaticHandler(server_conf_, NULL));
}

void Router::loadErrorPage(Connection &c, const ServerConf &conf) {
	const std::map<int, std::string> &pages = conf.error_pages;
	std::map<int, std::string>::const_iterator it = pages.find(c.res.getStatus());
	if (it != pages.end()) {
		const ServerConf &srv = conf;

		std::string rel = it->second; // ex: "/errors/404.html"
		std::string fullPath =
		    safe_join_under_root(srv.root, rel); // genre "www/errors/404.html"

		int fd = ::open(fullPath.c_str(), O_RDONLY);
		if (fd >= 0) {
			std::string body;
			char buf[4096];
			ssize_t r;
			while ((r = ::read(fd, buf, sizeof(buf))) > 0)
				body.append(buf, r);
			::close(fd);
			c.res.setBody(body);
			c.res.setContentType("text/html");
		} else {
			Logger::response("error_page configured for %d but file '%s' not found",
					 c.res.getStatus(), fullPath.c_str());
		}
	}
}