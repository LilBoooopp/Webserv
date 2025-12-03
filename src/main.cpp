#include "http/HttpParser.hpp"
#include "http/HttpRequest.hpp"
#include "utils/Path.hpp"
#include "utils/Colors.hpp"
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

	// uint32_t ip_be = htonl(INADDR_LOOPBACK); // 127.0.0.1
	// uint16_t port_be = htons(8080);

	Config config;
	Server s;

    if (getExtension(argv[1], '.') != "conf"){
	    std::cout << "Invalid configuration file:  " << argv[1] << " (must be .conf)\n";
        return 0;
    }
    if (!config.parse(argv[1]))
    {
        std::cerr << "Config error at line " << config.getErrorLine() << ": "
                <<  config.getErrorMessage() << std::endl;
        return (1);
    }

    config.debug_print();

    std::vector<ServerConf> servers = config.getServers();

	//uint16_t port_be = htons(servers[0].hosts[0].port);

	//struct sockaddr_in addr;
	//inet_aton(conf.servers[0].hosts[0].host, &addr.sin_addr);
	//htonl(addr.sin_addr);
	if (!s.start(servers[0].hosts[0].host, servers[0].hosts[0].port, servers)) {
		std::perror("webserv: start failed (is another instance running?");
		return (1);
	}
	s.setConf(servers);
	std::cout << "started" << std::endl;
	s.run();
	return (0);
}
