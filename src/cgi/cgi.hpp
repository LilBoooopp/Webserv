#pragma once
#include "../config/Config.hpp"
#include "../http/Connection.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

#include "../utils/Chrono.hpp"
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Path.hpp"
#include "../utils/Path.hpp"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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
	const Conf *cfg_;

    public:
	void runCgi(const HttpRequest &req, HttpResponse &res, Connection &c, int fd);
	bool handleResponses();
	void setConfig(const Conf &cfg);
};

void parseCgiRequest(const std::string &full, std::string &dir, std::string &file,
		     std::string &interpreter, std::string &queryString);
bool is_cgi(const std::string &req_target);

void placeFileInDir(const std::string &name, const std::string &fileContent,
		    const std::string &dir);