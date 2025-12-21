#include "Router.hpp"
#include "../utils/Logger.hpp"

const LocationConf *Router::matchLocation(const ServerConf &conf,
                                          const std::string &path) {
  const LocationConf *best_match = NULL;
  size_t longest_match_len = 0;

  for (size_t i = 0; i < conf.locations.size(); ++i) {
    const LocationConf &loc = conf.locations[i];
    const std::string &loc_path = loc.path;

    if (path.rfind(loc_path, 0) == 0) {
      bool is_valid_match = (loc_path.size() == path.size() ||
                             (loc_path.size() < path.size() &&
                              (path[loc_path.size() - 1] == '/')));

      if (is_valid_match && loc_path.size() > longest_match_len) {
        longest_match_len = loc_path.size();
        best_match = &loc;
      }
    }
  }
  return (best_match);
}

bool Router::checkAllowedMethod(Connection &c, const LocationConf &loc) {
  if (loc.methods.empty())
    return (true);

  const std::string &method = c.req.method;
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
    c.res.setStatusFromCode(405);
    std::string allow_header;
    for (size_t i = 0; i < loc.methods.size(); ++i) {
      if (i > 0)
        allow_header += ", ";
      allow_header += loc.methods[i];
    }
    c.res.setHeader("Allow", allow_header);
    return (false);
  }
  return (true);
}

bool Router::route(Connection &c) {
  const LocationConf *matched_loc = matchLocation(c.cfg, c.req.target);

  if (matched_loc) {
    c.loc = matched_loc;
    const LocationConf &loc = *matched_loc;
    Logger::router(
        "%smatched request's target to location \'%s%s%s\' in %s%s%s", GREY,
        YELLOW, loc.path.c_str(), GREY, TS, c.cfg.names[0].c_str(), GREY);

    if (!checkAllowedMethod(c, loc)) {
      Logger::router("unvalid method for \'%s\' location", loc.path.c_str());
      c.res.setStatusFromCode(405);
      c.res.ensureDefaultBodyIfEmpty();
      return true;
    }

    if (loc.redirect_enabled) {
      Logger::router("Redirecting \'%s\' to \'%s\' (%d).", c.req.target.c_str(),
                     loc.redirect_target.c_str(), loc.redirect_status);
      c.res.setStatusFromCode(loc.redirect_status);
      c.res.setHeader("Location", loc.redirect_target);
      return true;
    }
  } else
    Logger::router(
        "%sNo location match for \'%s%s%s\'. Using server defaults \'/\'.",
        GREY, TS, c.req.target.c_str(), GREY);
  return false;
}

void Router::loadErrorPage(Connection &c) {
  const std::map<int, std::string> &pages = c.cfg.error_pages;
  std::map<int, std::string>::const_iterator it = pages.find(c.res.getStatus());
  if (it != pages.end()) {
    const ServerConf &srv = c.cfg;

    std::string rel = it->second;
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

// Helper to resolve request path to filesystem path
std::string Router::resolvePath(const ServerConf &conf, const LocationConf *loc,
                                const std::string &target) {
  std::string request_path = target;
  std::string root_dir = conf.root;
  if (loc && loc->has_root) {
    size_t loc_path_len = loc->path.size();
    if (loc_path_len > 0 && target.compare(0, loc_path_len, loc->path) == 0)
      request_path.erase(0, loc_path_len);
  }
  if (!request_path.empty() && request_path[0] == '/')
    request_path.erase(0, 1);
  if (loc && loc->has_root)
    root_dir = loc->root;
  return safe_join_under_root(root_dir, request_path);
}

void Router::finalizeResponse(Connection &c) {
  if (c.res.getStatus() >= 400)
    Router::loadErrorPage(c);
  c.out = c.res.serialize(c.req.method == "HEAD");
}
