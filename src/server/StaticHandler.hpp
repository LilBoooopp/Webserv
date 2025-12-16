#pragma once
#include "../config/Config.hpp"
#include "IHandler.hpp"
#include <dirent.h>

struct StaticHandler : IHandler {
private:
  const ServerConf &server_conf_;
  const LocationConf *location_conf_;

public:
  explicit StaticHandler(const ServerConf &server_conf,
                         const LocationConf *location_conf = NULL)
      : server_conf_(server_conf), location_conf_(location_conf) {}
  virtual void handle(Connection &c, const HttpRequest &req, HttpResponse &res);
};
