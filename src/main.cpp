#include "http/HttpParser.hpp"
#include "http/HttpRequest.hpp"
#include "server/Server.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <cstdlib>
#include <iostream>
#include "config/Config.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

unsigned long IP_to_long(const char *addr)
{
	unsigned char a, b, c, d;
	sscanf(addr, "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d);
	return (a << 24 | b << 16 | c << 8 | d);
}

int main(int argc, char** argv) {
	(void)argc;
	if (argc <= 1)
		return (0);
	{
		HttpRequest rq;
		size_t endp = 0;
		std::string sample = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
		if (!HttpParser::parse(sample, rq, endp))
			return (1);
	}
	signal(SIGPIPE, SIG_IGN);

	uint32_t ip_be = htonl(INADDR_LOOPBACK); // 127.0.0.1
	// uint16_t port_be = htons(8080);

	Config config_cl;
	Conf	conf;
	Server s;

	conf = config_cl.parse(argv[1]);
	uint16_t port_be = htons(conf.servers[0].hosts[0].port);

	//struct sockaddr_in addr;
	//inet_aton(conf.servers[0].hosts[0].host, &addr.sin_addr);
	//htonl(addr.sin_addr);
	//ip_be = IP_to_long(conf.servers[0].hosts[0].host.c_str());

	if (!s.start(ip_be, port_be, conf)) {
		std::perror("webserv: start failed (is another instance running?");
		return (1);
	}
	std::cout << "started" << std::endl;
	s.run();
	return (0);
}
