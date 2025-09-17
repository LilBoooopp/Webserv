#include "server/Server.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpParser.hpp"
#include <arpa/inet.h>
#include <signal.h>

int	main(void) {
	{
		HttpRequest	rq;
		size_t		endp = 0;
		std::string	sample = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
		if (!HttpParser::parse(sample, rq, endp))
			return (1);
	}
	signal(SIGPIPE, SIG_IGN);
	Server	s;
	if (!s.start(htonl(INADDR_LOOPBACK), htons(8080)))
		return (1);
	s.run();
	return (0);
}
