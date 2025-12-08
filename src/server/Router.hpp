#pragma once
#include "../config/Config.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "IHandler.hpp"

class Router {
private:
  const ServerConf &server_conf_;

  const LocationConf *matchLocation(const std::string &path) const;
  bool checkAllowedMethod(const HttpRequest &req, const LocationConf &loc,
                          HttpResponse &res) const;

public:
  Router(const ServerConf &conf);
  IHandler *route(Connection &c, const HttpRequest &req, HttpResponse &res);
};
