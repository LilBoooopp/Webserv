#include "http/HttpParser.hpp"
#include "http/HttpRequest.hpp"
#include "server/Server.hpp"
#include <arpa/inet.h>
#include <signal.h>

int main(int argc, char **argv) {
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
	if (argc > 1)
		port_be = htons(std::atoi(argv[1]));
	Server s;
	if (!s.start(ip_be, port_be)) {
		std::perror("webserv: start failed (is another instance running?");
		return (1);
	}
	s.run();
	return (0);
}
