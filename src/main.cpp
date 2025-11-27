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

void print_server(const ServerConf &s)
{
    std::cout << "---- SERVER ----\n";

    std::cout << "Hosts:\n";
    for (size_t i = 0; i < s.hosts.size(); ++i)
        std::cout << "  - " << s.hosts[i].host << ":" << s.hosts[i].port << "\n";

    std::cout << "Server names:";
    for (size_t i = 0; i < s.names.size(); ++i)
        std::cout << " " << s.names[i];
    std::cout << "\n";

    std::cout << "Root: " << s.root << "\n";

    std::cout << "Index files:";
    for (size_t i = 0; i < s.files.size(); ++i)
        std::cout << " " << s.files[i];
    std::cout << "\n";

    std::cout << "Error pages:\n";
    for (std::map<int,std::string>::const_iterator it = s.error_pages.begin();
         it != s.error_pages.end(); ++it)
        std::cout << "  " << it->first << " => " << it->second << "\n";

    std::cout << "Max body size: " << s.max_size << "\n";

    std::cout << "Locations: " << s.locations.size() << "\n\n";
}

void print_location(const LocationConf &loc)
{
    std::cout << "  ---- LOCATION ----\n";
    std::cout << "  Path: " << loc.path << "\n";

    std::cout << "  Methods:";
    for (size_t i = 0; i < loc.methods.size(); ++i)
        std::cout << " " << loc.methods[i];
    std::cout << "\n";

    if (loc.redirect_enabled)
        std::cout << "  Redirect: " << loc.redirect_status
                  << " -> " << loc.redirect_target << "\n";

    if (loc.has_root)
        std::cout << "  Root override: " << loc.root << "\n";

    if (loc.has_index)
    {
        std::cout << "  Index override:";
        for (size_t i = 0; i < loc.index_files.size(); ++i)
            std::cout << " " << loc.index_files[i];
        std::cout << "\n";
    }

    if (loc.autoindex_set)
        std::cout << "  Autoindex: " << (loc.autoindex ? "on" : "off") << "\n";

    if (loc.upload_enabled)
        std::cout << "  Upload store: " << loc.upload_location << "\n";

    if (loc.has_py)
        std::cout << "  CGI Python: " << loc.py_path << "\n";

    if (loc.has_php)
        std::cout << "  CGI PHP: " << loc.php_path << "\n";

    if (loc.has_max_size)
        std::cout << "  Max body size override: " << loc.max_size << "\n";
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
	std::cout << "Parsed " << conf.servers.size() << " server(s)\n\n";

    for (size_t i = 0; i < conf.servers.size(); ++i)
    {
        print_server(conf.servers[i]);

        for (size_t j = 0; j < conf.servers[i].locations.size(); ++j)
        {
            print_location(conf.servers[i].locations[j]);
            std::cout << "\n";
        }
    }

	uint16_t port_be = htons(conf.servers[0].hosts[0].port);

	//struct sockaddr_in addr;
	//inet_aton(conf.servers[0].hosts[0].host, &addr.sin_addr);
	//htonl(addr.sin_addr);
	//ip_be = IP_to_long(conf.servers[0].hosts[0].host.c_str());

	if (!s.start(ip_be, port_be, conf)) {
		std::perror("webserv: start failed (is another instance running?");
		return (1);
	}
	s.setConf(conf);
	std::cout << "started" << std::endl;
	s.run();
	return (0);
}
