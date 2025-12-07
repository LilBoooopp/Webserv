#pragma once
#include "../config/Config.hpp"
#include "IHandler.hpp"

struct StaticHandler : IHandler {
  const std::vector<ServerConf> *cfg_;
  explicit StaticHandler(const std::vector<ServerConf> *cfg) : cfg_(cfg) {}
  virtual void handle(Connection &c, const HttpRequest &req, HttpResponse &res);
};
