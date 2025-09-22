#include "Server.hpp"

#include "../core/EpollReactor.hpp"
#include "Listener.hpp"

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/HttpParser.hpp"
#include "../http/ResponseWriter.hpp"

#include "StaticHandler.hpp"
#include "Router.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <map>


static int	set_nonblock(int fd) {
	int	flags = fcntl(fd, F_GETFL, 0);
	return ((flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0) ? 0 : -1);
}

// // tiny response helper
// static std::string	make_plain(int code, const char* reason, const char* body, bool closeit) {
// 	char	head[256];
// 	int		n = std::snprintf(head, sizeof(head),
// 		"HTTP/1.1 %d %s\r\n"
// 		"Content-Length: %zu\r\n"
// 		"Content-Type: text/plain\r\n"
// 		"Connection: %s\r\n"
// 		"\r\n",
// 		code, reason, std::strlen(body), closeit ? "close" : "keep-alive");
// 	return (std::string(head, head + n) + body);
// }

bool	Server::start(uint32_t ip_be, uint16_t port_be) {
	if (!listener_.bindAndListen(ip_be, port_be))
		return (false);
	if (!reactor_.add(listener_.fd(), EPOLLIN))
		return (false);
	return (true);
}

void	Server::run() {
	// Handlers & router (static site for now)
	StaticHandler	staticHandler(&cfg_);
	Router			router(&staticHandler);

	// main single-loop reactor
	while (true) {
		epoll_event	events[64];
		int	n = reactor_.wait(events, 64, -1);
		if (n <= 0)
			continue;

		for (int i = 0; i < n; ++i) {
			int	fd = events[i].data.fd;
			uint32_t	ev = events[i].events;

			// Listening socket: accept new clients
			if (fd == listener_.fd()) {
				if (ev & EPOLLIN) {
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

			// HUP/ERR: cleanup the client fd immediately
			if (ev & (EPOLLHUP | EPOLLERR)) {
				reactor_.del(fd);
				::close(fd);
				conns_.erase(fd);
				continue;
			}

			// READABLE path: pull bytes into per-connection buffer
			if (ev & EPOLLIN) {
				Connection	&c = conns_[fd];
				for (;;) {
					ssize_t	r = ::read(fd, &inbuf_[0], inbuf_.size());
					if (r > 0) {
						c.in.append(&inbuf_[0], r);

						// End of headers detected? Parse the head once.
						if (!c.headers_done && c.in.find("\r\n\r\n") != std::string::npos) {
							c.headers_done = true;

							HttpRequest	req;
							size_t	endpos = 0;
							if (HttpParser::parse(c.in, req, endpos)) {
								// Minimal validation/behavior
								HttpResponse	res;

								// Method & target checks
								if (req.method != "GET") {
									res.status = 501;
									res.reason = "Not Implemented";
									res.contentType = "text/plain";
									res.body = "not implemented";
									res.close = true;
								}
								else if (req.target.empty() || req.target[0] != '/')
								{
									res.status = 400;
									res.reason = "Bad Request";
									res.contentType = "text/plain";
									res.body = "bad request";
									res.close = true;
								}
								else
								{
									// Server static files form cfg_.root
									IHandler*	h = router.route(req.target);
									h->handle(req, res);
								}

								// Serialize response once
								c.out = ResponseWriter::toWire(res);
								c.close_after = res.close;

								// Consume the parsed head:
								c.in.erase(0, endpos);

								// Ensure writer notifications are active
								reactor_.mod(fd, EPOLLIN | EPOLLOUT);
							}
							else
							{
								// Malformed head -> 400
								HttpResponse	res;
								res.status = 400, res.reason = "Bad Request";
								res.contentType = "text/plain"; res.body = "bad request";
								res.close = true;

								c.out = ResponseWriter::toWire(res);
								c.close_after = res.close;

								reactor_.mod(fd, EPOLLIN | EPOLLOUT);
							}
						}
						continue;
					}
					if (r == 0)
					{
						c.peer_closed = true;
						break;
					}
					// r < 0 -> would block now
					break;
				}
			}

			// WRITABLE path: drain the out buffer
			if (ev & EPOLLOUT)
			{
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
					if (c.out.empty())
					{
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
