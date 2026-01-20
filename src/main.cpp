#include "config/Config.hpp"
#include "server/Server.hpp"
#include "utils/Path.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>

Server *g_server = NULL;
void signalHandler(int signum) {
	if (g_server) {
		std::cout << "\nReceived signal " << signum << ", cleaning up..." << std::endl;
		g_server->cleanup();
	}
	exit(signum);
}

std::string getDefaultConfig() {
	return "default.conf";
	std::cout << ".conf needed, use default.conf? ";
	std::string res;
	std::getline(std::cin, res);
	if (res.empty() || !std::strncmp(res.c_str(), "yes", 1)) {
		std::string def = "default.conf";
		std::cout << "Using default.conf\n";
		return "default.conf";
	}
	return "";
}

int main(int argc, char **argv) {
	std::string confPath = argc < 2 ? getDefaultConfig() : argv[1];
	if (confPath.empty() || getExtension(confPath, '.') != "conf") {
		std::cout << "Invalid configuration file:  \'" << confPath
			  << "\' (must be .conf)\n";
		return (1);
	}
	Config config;
	if (!config.parse(confPath)) {
		std::cerr << "Config error at line " << config.getErrorLine() << ": "
			  << config.getErrorMessage() << std::endl;
		return (1);
	}
	//config.debug_print();
	Server s(config);
	if (!s.start()) {
		std::perror("webserv: start failed (is another instance running?");
		return (1);
	}
	s.conf_path_ = confPath;

	g_server = &s;
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	s.run();
	return (0);
}
