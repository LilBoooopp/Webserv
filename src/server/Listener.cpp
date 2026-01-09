#include "Listener.hpp"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

static int set_nonblock(int fd) {
	int f = fcntl(fd, F_GETFL, 0);
	return ((f >= 0 && fcntl(fd, F_SETFL, f | O_NONBLOCK) == 0) ? 0 : -1);
}

Listener::Listener() : fd_(-1) {
	fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
	int on = 1;
	::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	#ifdef SO_REUSEPORT
		::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
	#endif
}

Listener::~Listener() {
  if (fd_ >= 0)
    ::close(fd_);
}

Listener::Listener(const Listener &other) {
	this->fd_ = other.fd_;
	other.fd_ = -1;
}

Listener &Listener::operator=(const Listener &other) {
	if (this != &other) {
		if (fd_ >= 0)
			::close(fd_);

		this->fd_ = other.fd_;
		other.fd_ = -1;
	}
	return (*this);
}

bool Listener::operator==(const int other) const { return (this->fd() == other); }

bool Listener::bindAndListen(uint32_t ip_be, uint16_t port_be, int backlog) {
	sockaddr_in sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = ip_be;
	sa.sin_port = port_be;
	if (::bind(fd_, (sockaddr *)&sa, sizeof(sa)) < 0)
		return (false);
	if (::listen(fd_, backlog) < 0)
		return (false);
	return (set_nonblock(fd_) == 0);
}
