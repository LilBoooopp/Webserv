#pragma once

#include "Cgi.hpp"

struct CgiData {
	pid_t pid;
	int readFd;
	int fd;
	int	tmp_fd;

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
	std::string	tmp_filename;

	Connection *conn;
	std::vector<pid_t> asyncPids_;

	bool tryInit(Connection &c, int fd);
	void log(bool execSuccess);

	CgiData();
};
