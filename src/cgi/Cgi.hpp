#pragma once

#include "../config/Config.hpp"
#include "../core/EpollReactor.hpp"
#include "../http/Connection.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../server/Router.hpp"
#include "../utils/Utils.hpp"
#include "CgiData.hpp"
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct CgiData;

class CgiHandler {
private:
  std::vector<CgiData> cgiResponses_;
  std::vector<pid_t> asyncPids_;
  const std::vector<ServerConf> *cfg_;
  EpollReactor *reactor_;

public:
  CgiHandler() : cfg_(NULL), reactor_(NULL) {}
  void init(EpollReactor *reactor) { reactor_ = reactor; }
  void setConfig(const std::vector<ServerConf> &cfg);

  bool runCgi(Connection &c, int fd);
  // bool handleResponses();
  void handleMessage(int fd);
  void checkCgiTimeouts();
  void killAsyncProcesses();
  void detachConnection(Connection *conn);

  bool hasFd(int fd);
};

bool is_cgi(const std::string &req_target, const ServerConf &cfg);
std::string getInterpreter(const std::string &path, const ServerConf &conf);
void parseCgiRequest(const std::string &target, std::string &dir,
                     std::string &file, std::string &queryString,
                     const ServerConf &conf);
