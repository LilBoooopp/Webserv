#include "Listener.hpp"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

static int	set_nonblock(int fd) {
	int f = fcntl(fd, F_GETFL, 0);
	return ((f >=0 && fcntl(fd, F_SETFL, f | O_NONBLOCK) == 0) ? 0 : -1);
}

Listener::Listener() : fd_(-1) {
	fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
	int	on = 1;
	::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

Listener::~Listener() {
	if (fd_ >= 0)
		::close(fd_);
}

bool	Listener::bindAndListen(uint32_t ip_be, uint16_t port_be, int backlog) {
	sockaddr_in	sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = ip_be;
	sa.sin_port = port_be;
	if (::bind(fd_, (sockaddr*)&sa, sizeof(sa)) < 0)
		return (false);
	if (::listen(fd_, backlog) < 0)
		return (false);
	return (set_nonblock(fd_) == 0);
}
