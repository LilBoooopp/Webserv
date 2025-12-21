#pragma once

#include "Cgi.hpp"

struct CgiData {
  pid_t pid;
  int readFd;
  int writeFd;
  size_t start;
  size_t bytesRead;

  std::vector<pid_t> asyncPids_;
  std::string method;
  std::string requestUri;
  std::string queryString;
  std::string contentLength;
  std::string contentType;
  std::string interp;
  std::string file;
  std::string path;

  Connection *conn;
  int fd;
  std::string out;
  bool noRead;
  CgiData()
      : pid(-1), readFd(-1), writeFd(-1), bytesRead(0), conn(NULL),
        noRead(false) {}
  bool tryInit(Connection &c, int fd);
  void log(bool execSuccess);
};
