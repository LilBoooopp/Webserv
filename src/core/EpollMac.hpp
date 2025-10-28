#pragma once
#include <stdint.h>
#include <map>
#include <vector>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#ifndef EPOLLIN
#define EPOLLIN  0x001
#endif
#ifndef EPOLLOUT
#define EPOLLOUT 0x004
#endif
#ifndef EPOLLERR
#define EPOLLERR 0x008
#endif
#ifndef EPOLLHUP
#define EPOLLHUP 0x010
#endif
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif
#ifndef EPOLLET
#define EPOLLET  0x80000000u
#endif
#ifndef EPOLLONESHOT
#define EPOLLONESHOT 0x40000000u
#endif

#ifndef EPOLL_CTL_ADD
#define EPOLL_CTL_ADD 1
#endif
#ifndef EPOLL_CTL_MOD
#define EPOLL_CTL_MOD 2
#endif
#ifndef EPOLL_CTL_DEL
#define EPOLL_CTL_DEL 3
#endif

typedef union { void* ptr; int fd; uint32_t u32; uint64_t u64; } epoll_data_t;
struct epoll_event { uint32_t events; epoll_data_t data; };

static inline int epoll_create(int)
{
	return kqueue();
}

struct __epoll_state {
	std::map<int, std::map<int,uint32_t> > masks;
};

static __epoll_state& __ep_get_state()
{
	static __epoll_state s;
	return s;
}

static void __ep_fill_changes(int fd, uint32_t events, std::vector<struct kevent>& ch, bool add, bool del)
{
	uint16_t flags = 0;
	if (add) flags |= EV_ADD | EV_ENABLE;
	if (del) flags |= EV_DELETE;
	if (events & EPOLLET) flags |= EV_CLEAR;
	if (events & EPOLLONESHOT) flags |= EV_ONESHOT;
	if (events & EPOLLIN) {
		struct kevent ev;
		EV_SET(&ev, fd, EVFILT_READ, flags, 0, 0, NULL);
		ch.push_back(ev);
	}
	if (events & EPOLLOUT) {
		struct kevent ev;
		EV_SET(&ev, fd, EVFILT_WRITE, flags, 0, 0, NULL);
		ch.push_back(ev);
	}
	if (!(events & (EPOLLIN|EPOLLOUT)) && (add || del)) {
		struct kevent ev;
		EV_SET(&ev, fd, EVFILT_READ, flags, 0, 0, NULL);
		ch.push_back(ev);
		EV_SET(&ev, fd, EVFILT_WRITE, flags, 0, 0, NULL);
		ch.push_back(ev);
	}
}

static inline int epoll_ctl(int epfd, int op, int fd, struct epoll_event* e)
{
	std::map<int, std::map<int,uint32_t> >& masks = __ep_get_state().masks;
	uint32_t prev = 0;
	if (masks[epfd].count(fd))
		prev = masks[epfd][fd];

	std::vector<struct kevent> ch;

	if (op == EPOLL_CTL_ADD) {
		if (masks[epfd].count(fd))
			op = EPOLL_CTL_MOD;
		else {
			uint32_t ev = e ? e->events : 0;
			__ep_fill_changes(fd, ev, ch, true, false);
			if (!ch.empty() && kevent(epfd, &ch[0], (int)ch.size(), NULL, 0, NULL) == -1)
				return -1;
			masks[epfd][fd] = ev;
			return 0;
		}
	}

	if (op == EPOLL_CTL_MOD) {
		__ep_fill_changes(fd, prev, ch, false, true);
		uint32_t ev = e ? e->events : 0;
		__ep_fill_changes(fd, ev, ch, true, false);
		if (!ch.empty() && kevent(epfd, &ch[0], (int)ch.size(), NULL, 0, NULL) == -1)
			return -1;
		masks[epfd][fd] = ev;
		return 0;
	}

	if (op == EPOLL_CTL_DEL) {
		__ep_fill_changes(fd, prev ? prev : (EPOLLIN|EPOLLOUT), ch, false, true);
		if (!ch.empty() && kevent(epfd, &ch[0], (int)ch.size(), NULL, 0, NULL) == -1) {
			if (errno != ENOENT)
				return -1;
		}
		masks[epfd].erase(fd);
		return 0;
	}

	errno = EINVAL;
	return -1;
}

static inline int epoll_wait(int epfd, struct epoll_event* out, int max, int timeout_ms)
{
	if (max <= 0 || !out) { errno = EINVAL; return -1; }

	std::vector<struct kevent> evs(max);
	struct timespec ts;
	struct timespec* pts = NULL;
	if (timeout_ms >= 0) {
		ts.tv_sec = timeout_ms / 1000;
		ts.tv_nsec = (long)(timeout_ms % 1000) * 1000000L;
		pts = &ts;
	}

	int n = kevent(epfd, NULL, 0, &evs[0], max, pts);
	if (n <= 0)
		return n;

	std::map<int, std::map<int,uint32_t> >& masks = __ep_get_state().masks;

	for (int i = 0; i < n; ++i) {
		out[i].events = 0;
		out[i].data.fd = (int)evs[i].ident;
		if (evs[i].filter == EVFILT_READ)
			out[i].events |= EPOLLIN;
		if (evs[i].filter == EVFILT_WRITE)
			out[i].events |= EPOLLOUT;
		if (evs[i].flags & EV_ERROR)
			out[i].events |= EPOLLERR;
		if (evs[i].flags & EV_EOF) {
			out[i].events |= EPOLLHUP;
			out[i].events |= EPOLLRDHUP;
		}
		if (masks.count(epfd) && masks[epfd].count((int)evs[i].ident)
		    && (masks[epfd][(int)evs[i].ident] & EPOLLET))
			out[i].events |= EPOLLET;
	}
	return n;
}
