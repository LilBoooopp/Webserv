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
  virtual void handle(Connection &c, const HttpRequest &req,
                      HttpResponse &res) {
    (void)c;
    (void)req;
    res.setStatusFromCode(status_code);
    res.ensureDefaultBodyIfEmpty();
  }
};

struct RedirectHandler : IHandler {
  int status_code;
  std::string target_url;
  RedirectHandler(int code, const std::string &url)
      : status_code(code), target_url(url) {}
  virtual void handle(Connection &c, const HttpRequest &req,
                      HttpResponse &res) {
    (void)c;
    (void)req;
    res.setStatusFromCode(status_code);
    res.setHeader("Location", target_url);
  }
};

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
    if (path.compare(0, loc.path.size(), loc.path) == 0) {
      if (path.size() == loc.path.size() || path[loc.path.size()] == '/') {
        if (loc.path.size() > longest_match_len || best_match == NULL) {
          longest_match_len = loc.path.size();
          best_match = &loc;
        }
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
  if (loc.methods.empty()) {
    if (req.method == "GET" || req.method == "HEAD")
      return (true);

    res.setStatusFromCode(405);
    res.setHeader("Allow", "GET, HEAD");
    return (false);
  }

  for (size_t i = 0; i < loc.methods.size(); ++i) {
    if (loc.methods[i] == req.method)
      return (true);
  }

  res.setStatusFromCode(405);
  std::string allow;
  for (size_t i = 0; i < loc.methods.size(); ++i) {
    if (i > 0)
      allow += ", ";
    allow += loc.methods[i];
  }
  res.setHeader("Allow", allow);
  return (false);
}

/**
 * @brief The main routing
 *
 * @param c
 * @param req
 * @param res
 * @return IHandler* The handler to process the request.
 */
IHandler *Router::route(Connection &c, const HttpRequest &req,
                        HttpResponse &res) {
  (void)c;
  const LocationConf *matched_loc = matchLocation(req.target);

  if (matched_loc) {
    const LocationConf &loc = *matched_loc;
    Logger::info("Router: Matched location: %s", loc.path.c_str());

    if (!checkAllowedMethod(req, loc, res)) {
      return (new ErrorHandler(405));
    }

    if (loc.redirect_enabled) {
      Logger::info("Router: Redirecting to %s (%d).",
                   loc.redirect_target.c_str(), loc.redirect_status);
      return (new RedirectHandler(loc.redirect_status, loc.redirect_target));
    }

    if (loc.has_max_size && c.body.size() > loc.max_size) {
      return new ErrorHandler(413);
    }

    Logger::info("Router: Using StaticHandler with location %s",
                 loc.path.c_str());
    return (new StaticHandler(server_conf_, matched_loc));
  }

  Logger::info("Router: No location matched. Using server defaults.");
  return (new StaticHandler(server_conf_, NULL));
}
