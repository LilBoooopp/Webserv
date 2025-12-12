#pragma once

#include "../cgi/Cgi.hpp"
#include "../config/Config.hpp"
#include "../core/EpollReactor.hpp"
#include "../http/Connection.hpp"
#include "Listener.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class CgiHandler;
class Server {
	EpollReactor reactor_;
	std::vector<Listener> listener_;
	std::map<int, Connection> conns_;
	std::vector<char> inbuf_;
	std::vector<ServerConf> cfg_;
	CgiHandler cgiHandler_;

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
	void cleanup();
};
