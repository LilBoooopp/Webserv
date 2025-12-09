#include "config/Config.hpp"
#include "http/HttpParser.hpp"
#include "server/Server.hpp"
#include "utils/Path.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>

std::string getDefaultConfig() {
	std::cout << "No config file given, would you like to use default.conf?" << std::endl;
	std::string res;
	std::cin >> res;
	if (!std::strncmp(res.c_str(), "yes", 1))
		return "default.conf";
	return "";
}

int main(int argc, char **argv) {
	std::string confPath = argc < 2 ? getDefaultConfig() : argv[1];
	if (confPath.empty())
		return (1);

	Config config;
	Server s;

	if (getExtension(confPath, '.') != "conf") {
		std::cout << "Invalid configuration file:  " << confPath << " (must be .conf)\n";
		return (1);
	}
	if (!config.parse(confPath)) {
		std::cerr << "Config error at line " << config.getErrorLine() << ": "
			  << config.getErrorMessage() << std::endl;
		return (1);
	}

	config.debug_print();

	std::vector<ServerConf> servers = config.getServers();

	if (!s.start(servers)) {
		std::perror("webserv: start failed (is another instance running?");
		return (1);
	}
	s.setConf(servers);
	std::cout << "started" << std::endl;
	s.run();
	return (0);
}
