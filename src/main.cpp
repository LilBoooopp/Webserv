#include "http/HttpParser.hpp"
#include "http/HttpRequest.hpp"
#include "server/Server.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>

int main(void) {
	{
		HttpRequest rq;
		size_t endp = 0;
		std::string sample = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
		if (!HttpParser::parse(sample, rq, endp))
			return (1);
	}
	signal(SIGPIPE, SIG_IGN);

	uint32_t ip_be = htonl(INADDR_LOOPBACK); // 127.0.0.1
	uint16_t port_be = htons(8080);

	Server s;
	if (!s.start(ip_be, port_be)) {
		std::perror("webserv: start failed (is another instance running?");
		return (1);
	}
	std::cout << "started" << std::endl;
	s.run();
	return (0);
}
