#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <signal.h>
#include <cctype>
#include <iostream>

struct Request {
	std::string							method, target, version;
	std::map<std::string, std::string>	headers;
};

struct Connection {
	std::string	in;				// bytes read so far
	std::string	out;			// bytes to write
	bool		headers_done;	// have we seen \r\n\r\n ?
	bool		responded;		// sent the reply already?
	bool		peer_closed;	// client set EOF
	bool		close_after;	// send Connection: close (for now always true)
	Connection(): headers_done(false), responded(false), peer_closed(false), close_after(true) {}
};

static	std::string	to_lower(const std::string& s)
{
	std::string	r = s;
	for (size_t i = 0; i < r.size(); ++i)
		r[i] = (char)std::tolower(r[i]);
	return (r);
}
static inline void rstrip_cr(std::string& s)
{
	if (!s.empty() && s[s.size() - 1] == '\r')
		s.erase(s.size() - 1);
}

static bool	parse_start_line(const std::string& line, Request& req)
{
	// Expect: METHOD SP TARGET SP HTTP/1.1
	size_t	p1 = line.find(' ');
	if (p1 == std::string::npos)
		return (false);
	size_t	p2 = line.find(' ', p1 + 1);
	if ( p2 == std::string::npos)
		return (false);
	req.method = line.substr(0, p1);
	req.target = line.substr(p1 + 1, p2 - (p1 + 1));
	req.version = line.substr(p2 + 1);
	return (req.version == "HTTP/1.1");
}

static bool	parse_headers_block(const std::string& block, Request& req) {
	size_t	start = 0;
	while (start < block.size())
	{
		size_t	end = block.find("\r\n", start);
		std::string	line = (end == std::string::npos)
			? block.substr(start)
			: block.substr(start, end - start);

		rstrip_cr(line);
		if (line.empty())
			break;
		size_t	colon = line.find(':');
		if (colon == std::string::npos)
			return (false);
		std::string key = to_lower(line.substr(0, colon));
		size_t v = colon + 1;
		while (v < line.size() && (line[v] == ' ' || line[v] == '\t'))
			++v;
		std::string	val = line.substr(v);
		req.headers[key] = val;
		if (end == std::string::npos)
			break;
		start = end + 2;
	}
	return (req.headers.find("host") != req.headers.end());
}

static bool	parse_http_request(const std::string& buf, Request& req, size_t& endpos)
{
	size_t	sep = buf.find("\r\n\r\n");
	if (sep == std::string::npos)
		return (false);
	std::string	head = buf.substr(0, sep);
	size_t	eol = head.find("\r\n");
	if (eol == std::string::npos)
		return (false);
	std::string	start_line = head.substr(0, eol);
	rstrip_cr(start_line);
	if (!parse_start_line(start_line, req))
		return (false);
	std::string	headers = head.substr(eol + 2);
	if (!parse_headers_block(headers, req))
		return (false);
	endpos = sep + 4;
	return (true);
}

static int	set_nonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	return ((flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0) ? 0 : -1);
}

int	main()
{
	{
		Request	rq;
		size_t	endp = 0;
		std::string	sample = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
		if (!parse_http_request(sample, rq, endp))
		{
			write(2, "parser_failed\n", 14);
			return (1);
		}
	}

	std::map<int, Connection> conns;
	signal(SIGPIPE, SIG_IGN);
	// listen socket
	int	ls = socket(AF_INET, SOCK_STREAM, 0);
	int	on = 1;

	setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	sockaddr_in	sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa.sin_port = htons(8080);
	bind(ls, (sockaddr*)&sa, sizeof(sa));
	listen(ls, 128);
	set_nonblock(ls);

	// epoll setup
	int	ep = epoll_create(128);
	epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = ls;
	epoll_ctl(ep, EPOLL_CTL_ADD, ls, &ev);
	std::vector<char>	inbuf(8192);
	const char	RESP_200[] =
		"HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nhello";
	const char	RESP_400[] =
		"HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nbad request";
	// single event loop
	while (true)
	{
		epoll_event	events[64];
		int	n = epoll_wait(ep, events, 64, -1);
		for (int i = 0; i < n; ++i)
		{
			int	fd = events[i].data.fd;
			if (fd == ls && (events[i].events & EPOLLIN))
			{
				// accept as many as ready
				for (;;)
				{
					int	c = accept(ls, 0, 0);
					if (c < 0)
						break;
					set_nonblock(c);
					epoll_event	cev;
					cev.events = EPOLLIN | EPOLLOUT;
					cev.data.fd = c;
					epoll_ctl(ep, EPOLL_CTL_ADD, c, &cev);
				}
			}
			else
			{
				//client socket ready
				if (events[i].events & EPOLLIN)
				{
					// read whatever arrived; ignore contents for now
					Connection	&c = conns[fd];
					Request		req;
					size_t		endpos = 0;
					for (;;)
					{
						ssize_t	r = read(fd, &inbuf[0], inbuf.size());
						if (r > 0)
						{
							c.in.append(&inbuf[0], r);
							if (!c.headers_done && c.in.find("\r\n\r\n") != std::string::npos)
							{
								c.headers_done = true;
								if (parse_http_request(c.in, req, endpos))
								{
									c.out.assign(RESP_200, sizeof(RESP_200) - 1);
									c.close_after = true;

									epoll_event	mod;
									mod.events = EPOLLIN | EPOLLOUT;
									mod.data.fd = fd;
									epoll_ctl(ep, EPOLL_CTL_MOD, fd, &mod);
								}
								else
								{
									c.out.assign(RESP_400, sizeof(RESP_400) - 1);
									c.close_after = true;

									epoll_event	mod;
									mod.events = EPOLLIN | EPOLLOUT;
									mod.data.fd = fd;
									epoll_ctl(ep, EPOLL_CTL_MOD, fd, &mod);
								}
							}
							continue;
						}
						if (r == 0)
						{
							c.peer_closed = true;
							break;
						}
						// r < 0 -> would block
						break;
					}
				}
				if ((events[i].events & EPOLLOUT) && conns[fd].out.size() && !conns[fd].responded)
				{
					// write our fixed HTTP response once, then close
					Connection	&c = conns[fd];
					ssize_t		sent = 0;
					while (sent < (ssize_t)c.out.size())
					{
						ssize_t	w = send(fd, c.out.data() + sent, c.out.size() - sent, MSG_NOSIGNAL);
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
						epoll_ctl(ep, EPOLL_CTL_DEL, fd, 0);
						close(fd);
						conns.erase(fd);
					}
				}
				if (events[i].events & (EPOLLHUP | EPOLLERR))
				{
					epoll_ctl(ep, EPOLL_CTL_DEL, fd, 0);
					close(fd);
					conns.erase(fd);
					continue;
				}
			}
		}
	}
}
