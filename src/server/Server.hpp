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

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>

class Server {
	EpollReactor				reactor_;
	Listener					listener_;
	std::map<int, Connection>	conns_;
	std::vector<char>			inbuf_;
	ServerConfig				cfg_;
public:
	Server(): inbuf_(8192) {}
	bool	start(uint32_t ip_be, uint16_t port_be);
	void	run();
};
