#include "Cgi.hpp"

static std::vector<char *> buildCgiEnv(CgiData &data, const ServerConf &cfg,
                                       std::vector<std::string> &envStrings) {
  envStrings.clear();

  envStrings.push_back("REDIRECT_STATUS=200");
  envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
  envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
  envStrings.push_back("SERVER_SOFTWARE=" + cfg.names[0]);
  envStrings.push_back("SERVER_NAME=" + cfg.hosts[0].host_str);
  envStrings.push_back("SERVER_PORT=" + to_string(cfg.hosts[0].port_int));
  envStrings.push_back("REQUEST_URI=" + data.requestUri);
  envStrings.push_back("REQUEST_METHOD=" + data.method);
  envStrings.push_back("QUERY_STRING=" + data.queryString);
  envStrings.push_back("CONTENT_LENGTH=" + data.contentLength);
  envStrings.push_back("CONTENT_TYPE=" + data.contentType);
  envStrings.push_back("SCRIPT_NAME=" + data.file);
  envStrings.push_back("SCRIPT_FILENAME=" + data.file);

  for (std::map<std::string, std::string>::iterator it =
           data.conn->req.headers.begin();
       it != data.conn->req.headers.end(); ++it) {
    std::string key = "HTTP_";
    for (size_t i = 0; i < it->first.size(); ++i) {
      char c = it->first[i];
      if (c == '-')
        key += '_';
      else
        key += std::toupper(static_cast<unsigned char>(c));
    }
    envStrings.push_back(key + "=" + it->second);
  }

  std::vector<char *> envp;
  for (size_t i = 0; i < envStrings.size(); ++i)
    envp.push_back(const_cast<char *>(envStrings[i].c_str()));
  envp.push_back(NULL);
  return envp;
}

static void closeFds(int *ins, int *outs) {
  if (ins) {
    close(ins[0]);
    close(ins[1]);
  }
  if (outs) {
    close(outs[0]);
    close(outs[1]);
  }
}

static void writeBody(const std::string &reqBody, int fd) {
  const char *p = reqBody.data();
  size_t to_write = reqBody.size();
  while (to_write > 0) {
    ssize_t w = ::write(fd, p, to_write);
    if (w > 0) {
      p += static_cast<size_t>(w);
      to_write -= static_cast<size_t>(w);
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      continue;
    break;
  }
  if (Logger::channels[LOG_BODY])
    Logger::cgi("[BODY] \'%s\' -> cgi", reqBody.c_str());
}

static bool execute(CgiData &data, const ServerConf &cfg,
                    const std::string &reqBody) {
  int outFds[2] = {-1, -1};
  int inFds[2] = {-1, -1};

  if (pipe(outFds) == -1 || pipe(inFds) == -1) {
    Logger::error("Pipe error");
    if (outFds[0] != -1)
      closeFds(inFds, outFds);
    return false;
  }

  std::vector<std::string> envStrings;
  std::vector<char *> envp = buildCgiEnv(data, cfg, envStrings);
  for (std::vector<char *>::const_iterator it = envp.begin(); it < envp.end();
       it++)
    if (*it)
      Logger::header("%s -> cgi", *it);

  pid_t pid = fork();
  if (pid == -1) {
    closeFds(inFds, outFds);
    Logger::error("Fork error");
    return false;
  }

  if (pid == 0) {
    if (dup2(outFds[1], STDOUT_FILENO) == -1)
      _exit(1);
    if (dup2(inFds[0], STDIN_FILENO) == -1)
      _exit(1);
    closeFds(inFds, outFds);
    chdir(data.path.c_str());

    const char *prog = data.interp.c_str();
    const char *argv[] = {prog, data.file.c_str(), NULL};
    execve(prog, const_cast<char *const *>(argv), &envp[0]);
    _exit(1);
  }
  close(outFds[1]);
  close(inFds[0]);
  if (!reqBody.empty())
    writeBody(reqBody, inFds[1]);
  close(inFds[1]);
  fcntl(outFds[0], F_SETFL, O_NONBLOCK);
  data.readFd = outFds[0];
  data.pid = pid;
  data.bytesRead = 0;
  data.start = now_ms();
  Logger::cgi("execution successfully started, added data to cgiResponses");
  return true;
}

bool CgiHandler::runCgi(Connection &c, int fd) {

  std::string interpreter = getInterpreter(c.req.target, c.cfg);
  if (interpreter.empty()) {
    Logger::cgi("No Interpreter found for cgi request's target \'%s\'",
                c.req.target.c_str());
    c.res.setStatus(500, "No Interpreter");
    c.res.setBody("no interpreter");
    c.res.setContentType("text/plain");
    return false;
  }

  CgiData data;
  if (!data.tryInit(c, fd)) {
    c.res.setStatus(404, "Not Found");
    c.res.setBody("not found");
    c.res.setContentType("text/plain");
    return false;
  }

  bool execSuccess = execute(data, c.cfg, c.body);

  if (execSuccess) {
    bool isAsync = c.req.hasHeader("x-async");
    if (isAsync) {
      close(data.readFd);
      c.res.setStatus(202, "Accepted");
      c.res.setBody("");
      c.res.setContentType("text/plain");
      asyncPids_.push_back(data.pid);
    } else {
      if (reactor_) {
        reactor_->add(data.readFd, EPOLLIN);
      }
      cgiResponses_.push_back(data);
    }
  } else {
    c.res.setStatus(500, "CGI Execution Failed");
    c.res.setBody("cgi error");
    c.res.setContentType("text/plain");
  }
  return true;
}

bool CgiHandler::hasFd(int fd) {
  for (size_t i = 0; i < cgiResponses_.size(); ++i) {
    if (cgiResponses_[i].readFd == fd)
      return (true);
  }
  return (false);
}
