#pragma once

#include "Cgi.hpp"

struct CgiData {
	std::vector<pid_t> asyncPids_;
	std::string method;
	std::string requestUri;
	std::string queryString;
	std::string contentLength;
	std::string contentType;
	std::string interp;
	std::string file;
	std::string path;
	std::map<std::string, std::string> headers;
	Connection *conn;
	int fd;
	int readFd;
	pid_t pid;
	size_t start;
	size_t bytesRead;
	std::string out;
	bool noRead;
	CgiData() : conn(NULL), readFd(-1), pid(-1), start(0), bytesRead(0), noRead(false) {}
	bool tryInit(Connection *c, const HttpRequest &req, int fd, const ServerConf &cfg);
	void log(bool execSuccess);
};
