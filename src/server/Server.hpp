#pragma once

#include "Listener.hpp"
#include "StaticHandler.hpp"
#include "Router.hpp"

#include "../core/EpollReactor.hpp"

#include "../config/Config.hpp"

#include "../http/Connection.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/HttpParser.hpp"
#include "../http/ResponseWriter.hpp"
#include "../http/ChunkedDecoder.hpp"

#include "../utils/Logger.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <ctime>
#include <iostream>

class Server {
	EpollReactor				reactor_;
	Listener					listener_;
	std::map<int, Connection>	conns_;
	std::vector<char>			inbuf_;
	ServerConfig				cfg_;

	void	acceptReady(std::time_t now);
	void	handleReadable(int fd, std::time_t now);
	void	handleWritable(int fd);
	void	enableWrite(int fd);
	void	disableWrite(int fd);
	void	prepareResponse(int fd, Connection& c);
public:
	Server(): inbuf_(8192) {}
	bool	start(uint32_t ip_be, uint16_t port_be);
	void	run();
};
