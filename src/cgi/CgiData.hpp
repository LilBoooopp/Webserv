#pragma once

#include "Cgi.hpp"

struct CgiData {
	pid_t pid;
	int readFd;
	int fd;

	size_t start;
	size_t bytesRead;
	size_t maxOutput;
	size_t timeout_ms;

	std::string method;
	std::string requestUri;
	std::string queryString;
	std::string contentLength;
	std::string contentType;
	std::string interp;
	std::string file;
	std::string path;
	std::string out;

	Connection *conn;
	std::vector<pid_t> asyncPids_;

	CgiData() : pid(-1), readFd(-1), bytesRead(0), maxOutput(-1), timeout_ms(-1), conn(NULL) {}
	bool tryInit(Connection &c, int fd);
	void log(bool execSuccess);
};
