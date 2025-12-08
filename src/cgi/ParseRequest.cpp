#include "../utils/Logger.hpp"
#include "cgi.hpp"

bool is_cgi(const std::string &req_target) {
  return req_target.rfind("/cgi/", 0) == 0;
}

static std::string getInterpreter(const std::string &path) {
  size_t pos = path.rfind('.');
  if (pos == std::string::npos)
    return "";
  std::string ext = path.substr(pos + 1);
  if (ext == "py")
    return "/usr/bin/python3";
  if (ext == "php")
    return "/usr/bin/php-cgi";
  return "";
}

static std::string extractArguments(std::string &pathToCgi) {
  std::string queryString = "";
  std::string::size_type qpos = pathToCgi.find('?');
  if (qpos == std::string::npos)
    return queryString;
  queryString = pathToCgi.substr(qpos + 1);
  pathToCgi = pathToCgi.substr(0, qpos);
  // Logger::info("args %s\"%s\"%s extracted from %s", GREY,
  // queryString.c_str(), TS, 	     pathToCgi.c_str());
  return queryString;
}

void parseCgiRequest(const std::string &req, std::string &dir,
                     std::string &file, std::string &interpreter,
                     std::string &queryString) {
  Logger::debug("Parsing cgi request: \'%s\'\n", req.c_str());
  std::string reqCopy(req);
  queryString = extractArguments(reqCopy);

  interpreter = getInterpreter(reqCopy);
  size_t pos = reqCopy.find_last_of('/');
  if (pos == std::string::npos) {
    dir = "";
    file = reqCopy;
    return;
  }
  dir = reqCopy.substr(0, pos + 1);
  file = reqCopy.substr(pos + 1);
}
