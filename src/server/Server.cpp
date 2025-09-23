#include "Server.hpp"

#include "../core/EpollReactor.hpp"
#include "Listener.hpp"

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/HttpParser.hpp"
#include "../http/ResponseWriter.hpp"
#include "../http/Connection.hpp"

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
	while (true)
	{
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
						Connection	c;
						conns_[cfd] = c;
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

			Connection	&c = conns_[fd];

			// READABLE path: pull bytes into per-connection buffer
			if (ev & EPOLLIN)
			{
				for (;;)
				{
					ssize_t	r = ::read(fd, &inbuf_[0], inbuf_.size());
					if (r > 0) {
						c.in.append(&inbuf_[0], r);

						// STATE: READING_HEADERS
						if (c.state == READING_HEADERS)
						{
							size_t	eoh = c.in.find("\r\n\r\n");
							if (eoh != std::string::npos)
							{
								c.headers_done = true;

								HttpRequest	req;
								size_t	endpos = 0;
								if (!HttpParser::parse(c.in, req, endpos))
								{
									// Malformed -> 400
									HttpResponse	res;
									res.status = 400;
									res.reason = "Bad Request";
									res.contentType = "text/plain";
									res.body = "bad request";
									c.out = ResponseWriter::toWire(res);
									c.close_after = res.close;
									c.state = WRITING_RESPONSE;
									reactor_.mod(fd, EPOLLIN | EPOLLOUT);
								}
								else
								{
									// Minimal validation
									bool	bad = false;
									if (req.version != "HTTP/1.1")
										bad = true;
									if (req.target.empty() || req.target[0] != '/')
										bad = true;
									if (bad)
									{
										HttpResponse	res;
										res.status = 400;
										res.reason = "Bad Request";
										res.contentType = "text/plain";
										res.body = "bad request";
										res.close = true;
										c.out = ResponseWriter::toWire(res);
										c.close_after = res.close;
										c.state = WRITING_RESPONSE;
										reactor_.mod(fd, EPOLLIN | EPOLLOUT);
									}
									else
									{
										// Save request onto the connection (used later when writing response)
										c.req = req;
										c.has_req = true;

										// Parse Content-Length
										size_t	content_length = 0;
										std::map<std::string, std::string>::iterator	itCL = req.headers.find("content-length");
										if (itCL != req.headers.end())
										{
											const std::string &s = itCL->second;
											size_t	acc = 0;
											for (size_t k = 0; k < s.size(); ++k)
											{
												if (s[k] < '0' || s[k] > '9')
												{
													acc = (size_t)-1;
													break;
												}
												acc = acc * 10 + (s[k] - '0');
											}
											if (acc == (size_t)-1)
											{
												HttpResponse	res;
												res.status = 400;
												res.reason = "Bad Request";
												res.contentType = "text/plain";
												res.body = "bad request";
												res.close = true;
												c.out = ResponseWriter::toWire(res);
												c.close_after = res.close;
												c.state = WRITING_RESPONSE;
												reactor_.mod(fd, EPOLLIN | EPOLLOUT);
												continue;
											}
											content_length = acc;
										}

										// Enforce client_max_body_size
										if (content_length > cfg_.client_max_body_size)
										{
											HttpResponse	res;
											res.status = 413;
											res.reason = "Payload Too Large";
											res.contentType = "text/plain";
											res.body = "payload too large";
											res.close = true;
											c.out = ResponseWriter::toWire(res);
											c.close_after = res.close;
											c.state = WRITING_RESPONSE;
											reactor_.mod(fd, EPOLLIN | EPOLLOUT);
											// Consume head to avoid re-parsing loops
											c.in.erase(0, endpos);
											continue;
										}

										// Move any bytes after head into body
										c.want_body = content_length;
										c.body.clear();

										if (c.in.size() > endpos)
										{
											size_t	avail = c.in.size() - endpos;
											size_t	take = (avail > c.want_body) ? c.want_body : avail;
											c.body.append(c.in.data() + endpos, take);
										}

										// Erase head ( + any bytes moved to body) from c.in
										c.in.erase(0, endpos + (c.body.size()));

										// Decide next state
										if (c.want_body == c.body.size())
											c.state = WRITING_RESPONSE;
										else
											c.state = READING_BODY;
									}
								}
							}
						}
					
						// STATE: READING_BODY
						if (c.state == READING_BODY)
						{
							// Pull form c.in into c.body until we reach want_body
							if (c.want_body > c.body.size() && !c.in.empty())
							{
								size_t	room = c.want_body - c.body.size();
								size_t	take = (c.in.size() > room) ? room : c.in.size();
								c.body.append(c.in.data(), take);
								c.in.erase(0, take);
							}
							if (c.body.size() == c.want_body)
								c.state = WRITING_RESPONSE;
						}

						// If we just became WRITING_RESPONSE and have no staged bytes yet, prepare response
						if (c.state == WRITING_RESPONSE && c.out.empty() && c.has_req)
						{
							const HttpRequest&	req = c.req;
							HttpResponse		res;

							if (req.method == "GET")
							{
								// Serve static files
								IHandler*	h = router.route(req.target);
								h->handle(req, res);
							}
							else if (req.method == "POST")
							{
								res.status = 200;
								res.reason = "OK";
								res.contentType = "text/plain";
								char	msg[128];
								std::snprintf(msg, sizeof(msg), "received %zu bytes\n", c.body.size());
								res.body = msg;
								res.close = true;
							}
							else
							{
								res.status = 501;
								res.reason = "Not Implemented";
								res.contentType = "text/plain";
								res.body = "not implemented";
								res.close = true;
							}

							c.out = ResponseWriter::toWire(res);
							c.close_after = res.close;
							reactor_.mod(fd, EPOLLIN | EPOLLOUT);
						}

						continue;
					}
					if (r == 0)
					{
						c.peer_closed = true;
						break;
					}
					break;
				}
			}

			// WRITABLE path: drain the out buffer
			if (ev & EPOLLOUT)
			{
				std::map<int, Connection>::iterator	it = conns_.find(fd);
				if (it == conns_.end())
					continue;
				
				Connection	&cx = it->second;
				if (!cx.out.empty() && !cx.responded) {
					ssize_t	sent = 0;
					while (sent < (ssize_t)cx.out.size()) {
						ssize_t	w = ::send(fd, cx.out.data() + sent, cx.out.size() - sent, MSG_NOSIGNAL);
						if (w > 0)
							sent += w;
						else
							break;
					}
					if (sent > 0)
						cx.out.erase(0, sent);
					if (cx.out.empty())
					{
						cx.responded = true;
						reactor_.del(fd);
						::close(fd);
						conns_.erase(it);
					}
				}
			}
		}
	}
}
