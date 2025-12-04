#pragma once
#include "Connection.hpp"

class HttpParser {
public:
  static bool parse(Connection &c, size_t &endpos);
};
