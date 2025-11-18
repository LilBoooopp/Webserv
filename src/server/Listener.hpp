#pragma once
#include <netinet/in.h>

class Listener {
	int	fd_;
public:
	Listener();
	~Listener();
	bool	bindAndListen(uint32_t ip_be, uint16_t port_be, int backlog = 128);
	int		fd() const { return (fd_); }
};
