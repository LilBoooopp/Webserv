#include "Server.hpp"

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/HttpParser.hpp"
#include "StaticHandler.hpp"
#include "Router.hpp"
#include "../http/ChunkedDecoder.hpp"

#include "../utils/Logger.hpp"

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

	const size_t	MAX_HANDLE_BYTES = 16 * 1024; // max bytes to read from socket per iteration
	const size_t	MAX_DECODE_BYTES = 16 * 1024; // max bytes to consume from c.in for body parsin per iteration

	size_t	handled = 0;
	while (handled < MAX_HANDLE_BYTES)
	{
		ssize_t	r = ::read(fd, &inbuf_[0], inbuf_.size());
		if (r > 0)
		{
			handled += static_cast<size_t>(r);
			c.in.append(&inbuf_[0], static_cast<size_t>(r));

			// Header cap defense
			//if (c.state == READING_HEADERS && c.in.size() > MAX_HANDLE_BYTES)
			//{
			//	HttpResponse	res(431);
			//	res.setContentType("text/plain");
			//	res.setBody("header too large");
			//	c.out = res.serialize(false);
			//	c.state = WRITING_RESPONSE;
			//	enableWrite(fd);
			//	c.in.clear();
			//	break;
			//}

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
						if (req.version != "HTTP/1.1" || req.version != "HTTP/1.0")
						{
							HttpResponse	res(505);
							res.setContentType("text/plain");
							res.setBody("HTTP Version Not Supported");
							c.out = res.serialize(false);
							c.state = WRITING_RESPONSE;
							enableWrite(fd);
							c.in.clear();
						}
						else
						{
							HttpResponse	res(400);
							res.setContentType("text/plain");
							res.setBody("Bad Request");
							c.out = res.serialize(false);
							c.state = WRITING_RESPONSE;
							enableWrite(fd);
							c.in.clear();
						}
					}
					else
					{
						bool	bad = false;

						// minimal validation
						if ((req.version != "HTTP/1.1" && req.version != "HTTP/1.0") || req.target.empty() || req.target[0] != '/')
							bad = true;
						
						// TE/CL detection
						bool	has_te_chunked = false;
						bool	has_cl = false;
						size_t	content_length = 0;

						// Check for Content-Length header
						std::map<std::string,std::string>::iterator	itCL = req.headers.find("content-length");
						if (itCL != req.headers.end() && !bad)
						{
							has_cl = true;
							const std::string &s = itCL->second;
							size_t acc = 0;
							for (size_t k = 0; k < s.size(); ++k)
							{
								if (s[k] < '0' || s[k] > '9')
								{
									acc = static_cast<size_t>(-1);
									break;
								}
								acc = acc * 10 + static_cast<size_t>(s[k] - '0');
							}
							if (acc == static_cast<size_t>(-1))
								bad = true;
							else
								content_length = acc;
						}

						// Check for Transfer-Encoding: chunked
						std::map<std::string,std::string>::iterator	itTE = req.headers.find("transfer-encoding");
						if (itTE != req.headers.end() && !bad)
						{
							std::string v = itTE->second;
							for (size_t k = 0; k < v.size(); ++k)
								v[k] = (char)std::tolower((unsigned char)v[k]);
							if (v.find("chunked") != std::string::npos)
								has_te_chunked = true;
						}

						// Having both CL and TE:chunked is invalid
						if (has_cl && has_te_chunked)
							bad = true;

						if (bad)
						{
							// Malformed or conflicint headers
							HttpResponse	res(400);
							res.setContentType("text/plain");
							res.setBody("Bad Request");
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

							// size limit for non-chunked CL bodies
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

								// Pre-consume body bytes for Content-Length requests
								if (!c.is_chunked && c.want_body > 0 && !c.in.empty())
								{
									size_t	take = (c.in.size() > c.want_body) ? c.want_body : c.in.size();
									c.body.append(c.in.data(), take);
									c.in.erase(0, take);
								}

								if (c.is_chunked)
								{
									// Initialize chunk decoder
									c.decoder.reset();
									c.state = READING_BODY;
								}
								else if (c.want_body == c.body.size()) // No Body or alread have entire body
									c.state = WRITING_RESPONSE;
								else // still need more body bytes
									c.state = READING_BODY;
							}
						}
						
					}
				}
			}

			// READING_BODY
			if (c.state == READING_BODY)
			{
				size_t	decode_left = MAX_DECODE_BYTES;

				if (c.is_chunked)
				{
					// Chunked transfer decoding loop
					while (decode_left > 0 && !c.in.empty())
					{
						size_t before = c.in.size();
						ChunkedDecoder::Status	st = c.decoder.feed(c.in, c.body);
						size_t consumed = before - c.in.size();
						if (consumed > decode_left)
							consumed = decode_left;
						decode_left -= consumed;

						if (st == ChunkedDecoder::NEED_MORE)
							break;
						if (st == ChunkedDecoder::ERROR)
						{
							HttpResponse	res(400);
							res.setContentType("text/plain");
							res.setBody("Bad Request");
							c.out = res.serialize(false);
							c.state = WRITING_RESPONSE;
							enableWrite(fd);
							break;
						}
						if (st == ChunkedDecoder::DONE)
						{
							// Final size check after full decoding
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
								c.state = WRITING_RESPONSE; // Full request body decoded
							break;
						}
						if (decode_left == 0)
							break;
					}
				}
				else
				{
					// Non-chunked body: consume up to want_body, capped by decode_left
					if (c.want_body > c.body.size() && !c.in.empty() && decode_left > 0)
					{
						size_t	room = c.want_body - c.body.size();
						size_t	take = c.in.size();
						if (take > room)
							take = room;
						if (take > decode_left)
							take = decode_left;

						c.body.append(c.in.data(), take);
						c.in.erase(0, take);
						decode_left -= take;
					}
					// If we now have the full body, we can move on to responding
					if (c.body.size() == c.want_body)
						c.state = WRITING_RESPONSE;
				}
			}

			// WRITING_RESPONSE
			if (c.state == WRITING_RESPONSE && c.out.empty() && c.has_req)
				prepareResponse(fd, c);

			// Continue reading (if handled < MAX_HANDLE_BYTES)
			// or exit the loop when the budget is used up
			continue;
		}
		if (r == 0)
		{
			c.peer_closed = true;
			break;
		}
		// r < 0
		// Subject does not allow checking errno so just stop and wait for epoll
		break;
	}
}

void	Server::handleWritable(int fd)
{
	// Find the connection associated with the fd
	std::map<int, Connection>::iterator	it = conns_.find(fd);
	if (it == conns_.end())
		return ;

	Connection	&c = it->second;

	// Max number of bytes we will attempt to send in a single call.
	// This prevents one big response from blocking other clients.
	const size_t MAX_WRITE_BYTES = 16 * 1024;

	// Only write if:
	// - c.out is not empty
	// - connection is not "responded"
	if (!c.out.empty() && !c.responded)
	{
		size_t	written = 0; // how many bytes we attempted/sent in this iteration
		ssize_t	offset = 0; // how many bytes from the fron of c.out we have consumed

		// Socket write loop
		while (written < MAX_WRITE_BYTES && offset < static_cast<ssize_t>(c.out.size()))
		{
			// Try to send as much as possible from c.out, starting at 'offset'.
			ssize_t	w = ::send(fd, c.out.data() + offset, c.out.size() - static_cast<size_t>(offset), MSG_NOSIGNAL);
			if (w > 0)
			{
				offset += w;
				written += static_cast<size_t>(w);
			}
			else
			{
				// w < 0 -> error
				// stop writing until epoll activates again
				break;
			}
		}

		// Drop the bytes we successfully sent from the front of c.out.
		if (offset > 0)
			c.out.erase(0, static_cast<size_t>(offset));

		if (c.out.empty())
		{
			c.responded = true;

			// Remove fd from reactor and close
			reactor_.del(fd);
			::close(fd);

			// Remove the connection stat from our map
			conns_.erase(it);
		}
	}
	else
	{
		// No data to write
		// remove EPOLLOUT so we dont get useless wakeups.
		disableWrite(fd);
	}
}

void	Server::run() {
	// main single-loop reactor
	while (true)
	{
		epoll_event	events[64];
		int	n = reactor_.wait(events, 64, -1);
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
	}
}
