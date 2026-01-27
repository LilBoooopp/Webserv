#pragma once

#include "../http/Connection.hpp"
#include "../cgi/Cgi.hpp"
#include "../config/Config.hpp"
#include "../core/EpollReactor.hpp"
#include "Listener.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>


class Server {
	EpollReactor reactor_;
	std::vector<Listener> listener_;
	std::map<int, Connection> conns_;
	std::vector<char> inbuf_;
	Config conf_;
	std::vector<ServerConf> cfg_;
	CgiHandler cgiHandler_;

	void acceptReady();
	void handleReadable(int fd);
	void handleWritable(int fd);
	void enableWrite(int fd);
	void disableWrite(int fd);
	void prepareResponse(int fd, Connection &c);
	void checkTimeouts();
	bool shouldKeepConnectionAlive(Connection &c);

    public:
		Server(Config conf) : inbuf_(8192), conf_(conf) {}
		bool start();
		int run();
		int executeStdin();
		void refresh();
		void cleanup();
		std::string conf_path_;
};
