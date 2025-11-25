#pragma once

#include "Listener.hpp"
#include "Router.hpp"
#include "StaticHandler.hpp"

#include "../cgi/cgi.hpp"
#include "../core/EpollReactor.hpp"
#include "../config/Config.hpp"

#include "../http/ChunkedDecoder.hpp"
#include "../http/Connection.hpp"
#include "../http/HttpParser.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/ResponseWriter.hpp"

#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "../http/ResponseWriter.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define MAX_READ 1024 * 16
#define MAX_WRITE 1024 * 16

class Server {
	EpollReactor reactor_;
	Listener listener_;
	std::map<int, Connection> conns_;
	std::vector<char> inbuf_;
	ServerConfig cfg_;
	cgiHandler cgiHandler_;

	void acceptReady();
	void handleReadable(int fd);
	void handleWritable(int fd);
	void enableWrite(int fd);
	void disableWrite(int fd);
	void prepareResponse(int fd, Connection &c);

    public:
	Server() : inbuf_(8192) {}
	bool start(uint32_t ip_be, uint16_t port_be);
	void run();
	void executeStdin();
};
