#pragma once
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include <map>
#include <string>

class HttpRequest {
public:
  std::map<std::string, std::string> headers;
  std::string method;
  std::string target;
  std::string version;

  HttpRequest();
  bool hasHeader(const std::string &key) const;
  bool eraseHeader(const std::string &key);
  std::string getHeader(const std::string &key) const;
  void removeBoundary(std::string &body, const std::string &boundary);

  void log() const;
};
