#pragma once

#include "Listener.hpp"

#include "../core/EpollReactor.hpp"

#include "../config/Config.hpp"

#include "../http/Connection.hpp"
#include "../http/ResponseWriter.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <ctime>

#define MAX_READ 1024 * 16
#define MAX_WRITE 1024 * 16

class Server {
	EpollReactor				reactor_;
	Listener					listener_;
	std::map<int, Connection>	conns_;
	std::vector<char>			inbuf_;
	ServerConfig				cfg_;

	void	acceptReady(void);
	void	handleReadable(int fd);
	void	handleWritable(int fd);
	void	enableWrite(int fd);
	void	disableWrite(int fd);
	void	prepareResponse(int fd, Connection& c);
public:
	Server(): inbuf_(8192) {}
	bool	start(uint32_t ip_be, uint16_t port_be);
	void	run();
};
