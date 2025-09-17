#include "Server.hpp"

#include "../http/HttpRequest.hpp"
#include "../http/HttpParser.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>

// local helper: set non-blocking
static int	set_nonblock(int fd) {
	int	flags = fcntl(fd, F_GETFL, 0);
	return ((flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0) ? 0 : -1);
}

// tiny response helper
static std::string	make_plain(int code, const char* reason, const char* body, bool closeit) {
	char	head[256];
	int		n = std::snprintf(head, sizeof(head),
		"HTTP/1.1 %d %s\r\n"
		"Content-Length: %zu\r\n"
		"Content-Type: text/plain\r\n"
		"Connection: %s\r\n"
		"\r\n",
		code, reason, std::strlen(body), closeit ? "close" : "keep-alive");
	return (std::string(head, head + n) + body);
}

bool	Server::start(uint32_t ip_be, uint16_t port_be) {
	if (!listener_.bindAndListen(ip_be, port_be))
		return (false);
	if (!reactor_.add(listener_.fd(), EPOLLIN))
		return (false);
	return (true);
}

void	Server::run() {
	// fixed wire responses
	const std::string	RESP_400 = make_plain(400, "Bad Request", "bad request", true);

	// main single-loop reactor
	while (true) {
		epoll_event	ev[64];
		int	n = reactor_.wait(ev, 64, -1);
		if (n <= 0)
			continue;

		for (int i = 0; i < n; ++i) {
			int	fd = ev[i].data.fd;
			uint32_t	events = ev[i].events;

			// accept path
			if (fd == listener_.fd()) {
				if (events & EPOLLIN) {
					for (;;) {
						int	cfd = ::accept(listener_.fd(), 0, 0);
						if (cfd < 0)
							break;
						set_nonblock(cfd);
						conns_[cfd] = Connection();
						reactor_.add(cfd, EPOLLIN | EPOLLOUT);
					}
				}
				continue;
			}

			// if we reach here its a client socket
			if (events & (EPOLLHUP | EPOLLERR)) {
				reactor_.del(fd);
				::close(fd);
				conns_.erase(fd);
				continue;
			}

			// READABLE
			if (events & EPOLLIN) {
				Connection	&c = conns_[fd];
				for (;;) {
					ssize_t	r = ::read(fd, &inbuf_[0], inbuf_.size());
					if (r > 0) {
						c.in.append(&inbuf_[0], r);

						if (!c.headers_done && c.in.find("\r\n\r\n") != std::string::npos) {
							c.headers_done = true;

							// parse head
							HttpRequest	req;
							size_t	endpos = 0;
							if (HttpParser::parse(c.in, req, endpos)) {
								// Minimal validation/behacior
								// - accept Get only
								// - require origin-form target starting with '/'
								int			code = 200;
								const char*	reason = "OK";
								const char*	body = "hello";
								if (req.method != "GET") {
									code = 501;
									reason = "Not Implemented";
									body = "not implemented";
								} else if (req.target.empty() || req.target[0] != '/') {
									code = 400;
									reason = "Bad Request";
									body = "bad request";
								} 
								c.out = make_plain(code, reason, body, true);
								c.close_after = true;

								c.in.erase(0, endpos);
								// Enable writer
								reactor_.mod(fd, EPOLLIN | EPOLLOUT);
							} else {
								// headers were present but invalid structure -> 400
								c.out = RESP_400;
								c.close_after = true;
								reactor_.mod(fd, EPOLLIN | EPOLLOUT);
							}
						}
						continue;
					}
					if (r == 0) { // peer sent EOF
						c.peer_closed = true;
						break;
					}
					// r < 0 -> would block now
					break;
				}
			}

			// WRITABLE
			if (events & EPOLLOUT) {
				std::map<int, Connection>::iterator	it = conns_.find(fd);
				if (it == conns_.end())
					continue;
				
				Connection	&c = it->second;
				if (!c.out.empty() && !c.responded) {
					ssize_t	sent = 0;
					while (sent < (ssize_t)c.out.size()) {
						ssize_t	w = ::send(fd, c.out.data() + sent, c.out.size() - sent, MSG_NOSIGNAL);
						if (w > 0)
							sent += w;
						else
							break;
					}
					if (sent > 0)
						c.out.erase(0, sent);
					if (c.out.empty()) {
						c.responded = true;
						reactor_.del(fd);
						::close(fd);
						conns_.erase(it);
					}
				}
			}
		}
	}
}
