#pragma once

#include "../config/Config.hpp"
#include "../http/Connection.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
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

    public:
	bool runCgi(const HttpRequest &req, HttpResponse &res, Connection &c, int fd);
	bool handleResponses();
	void setConfig(const std::vector<ServerConf> &cfg);
	void killAsyncProcesses();
	void detachConnection(Connection *conn);
};

bool is_cgi(const std::string &req_target, const ServerConf &cfg);
void placeFileInDir(const std::string &name, const std::string &fileContent,
		    const std::string &dir);
std::string getInterpreter(const std::string &path, const ServerConf &conf);
void parseCgiRequest(const std::string &target, std::string &dir, std::string &file,
		     std::string &queryString, const ServerConf &conf);