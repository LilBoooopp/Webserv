#pragma once
#include "../core/EpollReactor.hpp"
#include "Listener.hpp"
#include "../http/Connection.hpp"
#include <map>
#include <vector>

class Server {
	EpollReactor				reactor_;
	Listener					listener_;
	std::map<int, Connection>	conns_;
	std::vector<char>			inbuf_;
public:
	Server(): inbuf_(8192) {}
	bool	start(uint32_t ip_be, uint16_t port_be);
	void	run();
};
