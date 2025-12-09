#pragma once
#include "../http/Connection.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

struct IHandler {
  virtual ~IHandler() {}
  virtual void handle(Connection &c, const HttpRequest &req,
                      HttpResponse &res) = 0;
};
