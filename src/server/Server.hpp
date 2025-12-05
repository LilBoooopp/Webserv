#pragma once

#include "../cgi/cgi.hpp"
#include "../config/Config.hpp"
#include "../core/EpollReactor.hpp"
#include "../http/Connection.hpp"
#include "../http/ResponseWriter.hpp"
#include "Listener.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define MAX_READ 1024 * 16
#define MAX_WRITE 1024 * 16

class Server {
  EpollReactor reactor_;
  std::vector<Listener> listener_;
  std::map<int, Connection> conns_;
  std::vector<char> inbuf_;
  std::vector<ServerConf> cfg_;
  cgiHandler cgiHandler_;

  void acceptReady();
  void handleReadable(int fd);
  void handleWritable(int fd);
  void enableWrite(int fd);
  void disableWrite(int fd);
  void prepareResponse(int fd, Connection &c);

public:
  Server() : inbuf_(8192) {}
  bool start(std::vector<ServerConf> &config);
  void setConf(std::vector<ServerConf> config);
  void run();
  bool executeStdin();
};
