#include "Server.hpp"

/**
 * @return pointer or 0 if gone
 */
inline Connection* Server::find_conn(int fd)
{
	std::map<int, Connection>::iterator it = conns_.find(fd);
	return ((it == conns_.end()) ? 0 : &it->second);
}

inline void Server::hard_close(int fd)
{
	reactor_.del(fd);
	::close(fd);
	conns_.erase(fd);
}

inline void Server::send_408_and_send(int fd, Connection& c)
{
	HttpResponse res(408);
	res.setContentType("text/plain");
	res.setBody("request timeout");
	c.out = res.serialize(false);
	c.state = WRITING_RESPONSE;
	enableWrite(fd);

	c.gen_send++;
	timers_.add(fd, T_SEND, add_ms(now_ms(), cfg_.timeouts.send_timeout_ms), c.gen_send);
}

/**
 * @brief Check if persistant connection
 */
static bool	wants_close_after(const HttpRequest& req)
{
	// keep persistent by default if HTTP/1.1
	std::map<std::string,std::string>::const_iterator it = req.headers.find("connection");
	if (it != req.headers.end())
	{
		std::string v = it->second;
		for (size_t i = 0; i < v.size(); ++i)
			v[i] = (char)std::tolower((unsigned char)v[i]);
		if (v.find("close") != std::string::npos)
			return (true);
	}
	// HTTP/1.0 non persistent unless said otherwise
	if (req.version == "HTTP/1.0")
	{
		if (it != req.headers.end())
		{
			const std::string& v = it->second;
			if (v.find("keep-alive") != std::string::npos)
				return (false);
		}
		return (true);
	}
	return (false);
}

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
	reactor_.add(STDIN_FILENO, EPOLLIN);
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

		conns_[cfd].gen_header++;
		timers_.add(cfd, T_HEADER, add_ms(now_ms(), cfg_.timeouts.client_header_timeout_ms), conns_[cfd].gen_header);
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

	if (c.close_after)
		res.setHeader("Connection", "close");
	else
		res.setHeader("Connection", "keep-alive");

	c.out = res.serialize(head_only);
	c.gen_send++;
	timers_.add(fd, T_SEND, add_ms(now_ms(), cfg_.timeouts.send_timeout_ms), c.gen_send);
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
			c.gen_header++;
			timers_.add(fd, T_HEADER, add_ms(now_ms(), cfg_.timeouts.client_header_timeout_ms), c.gen_header);

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
									c.gen_body++;
									timers_.add(fd, T_BODY, add_ms(now_ms(), cfg_.timeouts.client_body_timeout_ms), c.gen_body);
									c.in.erase(0, take);
								}

								if (c.is_chunked)
								{
									c.decoder.reset();
									c.state = READING_BODY;
									c.gen_body++;
									timers_.add(fd, T_BODY, add_ms(now_ms(), cfg_.timeouts.client_body_timeout_ms), c.gen_body);
								}
								else if (c.want_body == c.body.size())
									c.state = WRITING_RESPONSE;
								else
									c.state = READING_BODY;
								c.close_after = wants_close_after(req);
							}
						}
						
					}
				}
				c.gen_header++;
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
			{
				prepareResponse(fd, c);
				c.gen_send++;
				timers_.add(fd, T_SEND, add_ms(now_ms(), cfg_.timeouts.send_timeout_ms), c.gen_send);
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
			{
				sent += w;
				
				c.gen_send++;
				timers_.add(fd, T_SEND, add_ms(now_ms(), cfg_.timeouts.send_timeout_ms), c.gen_send);
			}
			else
				break;
		}
		if (sent > 0)
			c.out.erase(0, sent);

		if (c.out.empty())
		{
			c.responded = true;
			c.gen_send++;

			if (!c.close_after && cfg_.keepalive)
			{
				// keep alive mode
				// 1) reset connection state to parse next request
				c.in.clear();
				c.body.clear();
				c.headers_done = false;
				c.responded = false;
				c.peer_closed = false;
				c.has_req = false;
				c.is_chunked = false;
				c.want_body = 0;
				c.state = READING_HEADERS;
				c.decoder.reset();

				// 2) back to read events only
				disableWrite(fd);

				// 3) refresh keepalive inactivity timer
				c.gen_keep++;
				timers_.add(fd, T_KEEPALIVE, add_ms(now_ms(), cfg_.timeouts.keepalive_timeout_ms), c.gen_keep);
			}
			else
			{
				reactor_.del(fd);
				::close(fd);
				conns_.erase(it);
			}
		}
	}
	else
		disableWrite(fd);
}

void	Server::run() {
	// main single-loop reactor
	while (true)
	{
		epoll_event	events[64];
		int	poll_timeout = timers_.time_to_next(now_ms());
		int	n = reactor_.wait(events, 64, poll_timeout);
		if (n <= 0)
			continue;

		std::time_t	now = std::time(NULL);

		for (int i = 0; i < n; ++i)
		{
			int			fd = events[i].data.fd;
			uint32_t	ev = events[i].events;

			// Check for an input in console to quit correctly (NEED TO UPDATE WHEN CGI)
			if (fd == 0)
			{
				ssize_t	r = ::read(fd, &inbuf_[0], inbuf_.size());
				if (r > 0)
					return ;
			}

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

		// TIMER EXPERIATION
		Timer	t;
		u64		now_ms_val = now_ms();
		while (timers_.next_due(now_ms_val, t))
		{
			Connection* cp = find_conn(t.fd);
			if (!cp) // fd already closed
				continue;
			Connection&	c = *cp;

			// Ignore stale timers "generation mismatch"
			if ((t.kind == T_HEADER && t.gen != c.gen_header) ||
				(t.kind == T_BODY && t.gen != c.gen_body) ||
				(t.kind == T_SEND && t.gen != c.gen_send) ||
				(t.kind == T_KEEPALIVE && t.gen != c.gen_keep))
			{
				continue;
			}

			switch (t.kind)
			{
				case T_HEADER:
					// reading headers without progress
					if (c.state == READING_HEADERS && !c.responded)
						send_408_and_send(t.fd, c);
					else
						hard_close(t.fd);
					break;
				
				case T_BODY:
					// reading body without progress
					if (c.state == READING_BODY && !c.responded)
						send_408_and_send(t.fd, c);
					else
						hard_close(t.fd);
					break;

				case T_SEND:
					// no send progress
					hard_close(t.fd);
					break;

				case T_KEEPALIVE:
					// no new request arrived in time
					hard_close(t.fd);
					break;
			}
		}
	}
}
