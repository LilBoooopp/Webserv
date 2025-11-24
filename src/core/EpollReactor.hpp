#pragma once
#ifndef MAC
#include "EpollMac.hpp"
#else
#include <sys/epoll.h>
#endif

#include <map>

class EpollReactor {
	int epfd_;

    public:
	EpollReactor();
	~EpollReactor();
	bool add(int fd, uint32_t events);
	bool mod(int fd, uint32_t events);
	bool del(int fd);
	int wait(epoll_event *out, int max, int timeout_ms);
	int fd() const { return epfd_; }
};
