#pragma once
#include "../config/Config.hpp"
#include "../http/Connection.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>

struct CgiExecutionData {
	std::string interpreter;
	std::string path;
	std::string file;
	std::string queryString;
	Connection *conn;
	int fd;
	int readFd;
	pid_t pid;
	size_t start;
	size_t bytesRead;
	std::string out;
	CgiExecutionData() : conn(NULL), readFd(-1), pid(-1), start(0), bytesRead(0) {}
};

class cgiHandler {
    private:
	std::vector<CgiExecutionData> cgiResponses_;
	const ServerConfig *cfg_;

    public:
	void runCgi(const HttpRequest &req, HttpResponse &res, Connection &c, int fd);
	bool handleResponses();
	void setConfig(const ServerConfig &cfg);
};

void parseCgiRequest(const std::string &full, std::string &dir, std::string &file,
		     std::string &interpreter, std::string &queryString);
bool is_cgi(const std::string &req_target);