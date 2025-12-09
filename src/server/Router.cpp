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
IHandler *Router::route(Connection &c, const HttpRequest &req,
                        HttpResponse &res) {
  (void)c;
  const LocationConf *matched_loc = matchLocation(req.target);

  if (matched_loc) {
    const LocationConf &loc = *matched_loc;
    Logger::debug("Router: Matched location: %s", loc.path.c_str());

    if (!checkAllowedMethod(req, loc, res)) {
      return (new ErrorHandler(405));
    }

    if (loc.redirect_enabled) {
      Logger::debug("Router: Redirecting to %s (%d).",
                    loc.redirect_target.c_str(), loc.redirect_status);
      return (new RedirectHandler(loc.redirect_status, loc.redirect_target));
    }

    Logger::debug("Router: Using StaticHandler with location %s",
                  loc.path.c_str());
    return (new StaticHandler(server_conf_, matched_loc));
  }

  Logger::debug("Router: No location matched. Using server defaults.");
  return (new StaticHandler(server_conf_, NULL));
}
