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
	Connection *conn;
	int fd;
	int readFd;
	pid_t pid;
	size_t start;
	size_t bytesRead;
	std::string out;
	bool noRead;
	CgiData() : conn(NULL), readFd(-1), pid(-1), start(0), bytesRead(0), noRead(false) {}
	bool tryInit(Connection &c, int fd);
	void log(bool execSuccess);
};
