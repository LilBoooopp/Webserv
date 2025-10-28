#include "Server.hpp"

void	Server::enableWrite(int fd) { reactor_.mod(fd, EPOLLIN | EPOLLOUT); }
void	Server::disableWrite(int fd) { reactor_.mod(fd, EPOLLIN); }

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

void	Server::acceptReady(std::time_t now)
{
	if (listener_.fd() < 0)
		return ;
	for (;;)
	{
		int cfd = ::accept(listener_.fd(), 0, 0);
		if (cfd < 0)
			break;
		set_nonblock(cfd);
		Connection	c;
		c.last_active = now;
		conns_[cfd] = c;
		reactor_.add(cfd, EPOLLIN);
		Logger::debug("accepted fd=%d", cfd);
	}
}

// Build and serialize a response once the request/body are ready
void	Server::prepareResponse(int fd, Connection& c)
{
	const HttpRequest&	req = c.req;

	HttpResponse	res(200);
	bool			head_only = (req.method == "HEAD");

	if (req.method == "GET" || req.method == "HEAD")
	{
		StaticHandler	StaticHandler(&cfg_);
		Router			router(&StaticHandler);
		router.route(req.target)->handle(req, res);
	}
	else if (req.method == "POST") // TEMP, only echo response
	{
		res.setContentType("text/plain");
		char	msg[128];
		std::snprintf(msg, sizeof(msg), "received %zu bytes\n", c.body.size());
		res.setBody(msg);
	}
	else
	{
		res = HttpResponse(501);
		res.setContentType("text/plain");
		res.setBody("not implemented");
	}

	c.out = res.serialize(head_only);
	enableWrite(fd);
}

void	Server::handleReadable(int fd, std::time_t now)
{
	Connection &c = conns_[fd];
	c.last_active = now;

	const size_t	MAX_HANDLE_BYTES = 16 * 1024;

	for (;;)
	{
		ssize_t	r = ::read(fd, &inbuf_[0], inbuf_.size());
		if (r > 0)
		{
			c.in.append(&inbuf_[0], r);

			// Header cap defense
			if (c.state == READING_HEADERS && c.in.size() > MAX_HANDLE_BYTES)
			{
				HttpResponse	res(431);
				res.setContentType("text/plain");
				res.setBody("header too large");
				c.out = res.serialize(false);
				c.state = WRITING_RESPONSE;
				enableWrite(fd);
				c.in.clear();
				break;
			}

			// READING_HEADERS
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
						HttpResponse	res(400);
						res.setContentType("text/plain");
						res.setBody("bad request");
						c.out = res.serialize(false);
						c.state = WRITING_RESPONSE;
						enableWrite(fd);
						c.in.clear();
					}
					else
					{
						// minimal validation
						bool	bad = false;
						if (req.version != "HTTP/1.1" || req.target.empty() || req.target[0] != '/')
							bad = true;
						
						// TE/CL detection
						bool	has_te_chunked = false;
						bool	has_cl = false;
						size_t	content_length = 0;

						std::map<std::string,std::string>::iterator	itCL = req.headers.find("content-length");
						if (itCL != req.headers.end())
						{
							has_cl = true;
							const std::string &s = itCL->second;
							size_t acc = 0;
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
								bad = true;
							else
								content_length = acc;
						}
						std::map<std::string,std::string>::iterator	itTE = req.headers.find("transfer-encoding");
						if (itTE != req.headers.end())
						{
							std::string v = itTE->second;
							for (size_t k = 0; k < v.size(); ++k)
								v[k] = (char)std::tolower((unsigned char)v[k]);
							if (v.find("chunked") != std::string::npos)
								has_te_chunked = true;
						}
						if (has_cl && has_te_chunked)
							bad = true;

						if (bad)
						{
							HttpResponse	res(400);
							res.setContentType("text/plain");
							res.setBody("bad request");
							c.out = res.serialize(false);
							c.state = WRITING_RESPONSE;
							enableWrite(fd);
							c.in.erase(0, endpos);
						}
						else
						{
							c.req = req;
							c.has_req = true;
							c.is_chunked = has_te_chunked;
							c.want_body = has_cl ? content_length : 0;
							c.body.clear();

							// size limit for CL
							if (has_cl && c.want_body > cfg_.client_max_body_size)
							{
								HttpResponse	res(413);
								res.setContentType("text/plain");
								res.setBody("payload too large");
								c.out = res.serialize(false);
								c.state = WRITING_RESPONSE;
								enableWrite(fd);
								c.in.erase(0, endpos);
							}
							else
							{
								// Remove head so that only the body is leftover
								c.in.erase(0, endpos);

								if (!c.is_chunked && c.want_body > 0 && !c.in.empty())
								{
									size_t	take = (c.in.size() > c.want_body) ? c.want_body : c.in.size();
									c.body.append(c.in.data(), take);
									c.in.erase(0, take);
								}

								if (c.is_chunked)
								{
									c.decoder.reset();
									c.state = READING_BODY;
								}
								else if (c.want_body == c.body.size())
									c.state = WRITING_RESPONSE;
								else
									c.state = READING_BODY;
							}
						}
						
					}
				}
			}

			// READING_BODY
			if (c.state == READING_BODY)
			{
				if (c.is_chunked)
				{
					for (;;)
					{
						ChunkedDecoder::Status	st = c.decoder.feed(c.in, c.body);
						if (st == ChunkedDecoder::NEED_MORE)
							break;
						if (st == ChunkedDecoder::ERROR)
						{
							HttpResponse	res(400);
							res.setContentType("text/plain");
							res.setBody("bad request");
							c.out = res.serialize(false);
							c.state = WRITING_RESPONSE;
							enableWrite(fd);
							break;
						}
						if (st == ChunkedDecoder::DONE)
						{
							if (c.body.size() > cfg_.client_max_body_size)
							{
								HttpResponse	res(413);
								res.setContentType("text/plain");
								res.setBody("payload too large");
								c.out = res.serialize(false);
								c.state = WRITING_RESPONSE;
								enableWrite(fd);
							}
							else
								c.state = WRITING_RESPONSE;
							break;
						}
					}
				}
				else
				{
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
			}

			// WRITING_RESPONSE
			if (c.state == WRITING_RESPONSE && c.out.empty() && c.has_req)
				prepareResponse(fd, c);

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

void	Server::handleWritable(int fd)
{
	std::map<int, Connection>::iterator	it = conns_.find(fd);
	if (it == conns_.end())
		return ;

	Connection	&c = it->second;
	if (!c.out.empty() && !c.responded)
	{
		ssize_t	sent = 0;
		while (sent < (ssize_t)c.out.size())
		{
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
	else
		disableWrite(fd);
}

void	Server::run() {
	// main single-loop reactor
	while (true)
	{
		std::cout << "====== READING ======\n";
		epoll_event	events[64];
		int	n = reactor_.wait(events, 64, -1);
		if (n <= 0)
			continue;

		std::time_t	now = std::time(NULL);

		for (int i = 0; i < n; ++i)
		{
			int			fd = events[i].data.fd;
			uint32_t	ev = events[i].events;

			if (fd == listener_.fd())
			{
				if (ev & EPOLLIN)
					acceptReady(now);
				continue;
			}

			if (ev & (EPOLLHUP | EPOLLERR))
			{
				reactor_.del(fd);
				::close(fd);
				conns_.erase(fd);
				continue;
			}

			if (ev & EPOLLIN)
				handleReadable(fd, now);
			if (ev & EPOLLOUT)
				handleWritable(fd);
		}
	}
}
