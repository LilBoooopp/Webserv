#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "Config.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

void Config::debug_print_location(const LocationConf &location, const char *clr) {
	std::cout << "        Location: " << location.path << std::endl;
	std::cout << "            Methods:";
	for (size_t i = 0; i < location.methods.size(); ++i)
		std::cout << " " << location.methods[i];
	std::cout << std::endl;

	if (location.redirect_enabled)
		std::cout << "            Redirect: " << location.redirect_status << " "
			  << location.redirect_target << std::endl;
	if (location.has_root)
		std::cout << "            Root override: " << location.root << std::endl;
	if (location.has_index) {
		std::cout << "            Index override:";
		for (size_t i = 0; i < location.index_files.size(); ++i)
			std::cout << " " << location.index_files[i];
		std::cout << std::endl;
	}
	if (location.autoindex_set)
		std::cout << "             Autoindex: " << (location.autoindex ? "on" : "off")
			  << std::endl;
	if (location.upload_enabled)
		std::cout << "             Uploads: " << location.upload_location << std::endl;
	if (!location.cgi.empty()) {
		std::cout << "             CGIs:" << std::endl;
		for (std::map<std::string, std::string>::const_iterator it = location.cgi.begin();
		     it != location.cgi.end(); ++it)
			std::cout << "            " << it->first << " : " << it->second
				  << std::endl;
	}
	if (location.has_max_size)
		std::cout << clr << "            Max Size override: " << GREY
			  << bytesToStr(location.max_size) << std::endl;
	if ((int)location.cgi_maxOutput != -1)
		std::cout << clr << "            Cgi Max Output: " << GREY
			  << bytesToStr(location.cgi_maxOutput) << std::endl;
	if ((int)location.cgi_timeout_ms != -1)
		std::cout << clr << "            Cgi Timeout: " << GREY
			  << timeToStr(location.cgi_timeout_ms) << std::endl;
}

void Config::debug_print_server(const ServerConf &server, const char *clr) {
	std::cout << "Server:" << std::endl;
	std::cout << "    Hosts:" << std::endl;
	for (size_t i = 0; i < server.hosts.size(); ++i)
		std::cout << " " << server.hosts[i].host << ":" << server.hosts[i].port
			  << std::endl;
	std::cout << "    Names:";
	for (size_t i = 0; i < server.names.size(); ++i)
		std::cout << " " << server.names[i];
	std::cout << std::endl;
	std::cout << "    Root: " << server.root << std::endl;
	std::cout << "    Index:";
	for (size_t i = 0; i < server.files.size(); ++i)
		std::cout << " " << server.files[i];
	std::cout << std::endl;
	std::cout << clr << "    Error pages:" << GREY << std::endl;
	for (std::map<int, std::string>::const_iterator it = server.error_pages.begin();
	     it != server.error_pages.end(); ++it)
		std::cout << "        " << it->first << " " << it->second << std::endl;
	std::cout << std::endl;
	std::cout << clr << "    Max body size: " << GREY << bytesToStr(server.max_size)
		  << std::endl;
	std::cout << clr << "    Locations:" << GREY << std::endl;
	for (size_t i = 0; i < server.locations.size(); ++i)
		debug_print_location(server.locations[i], clr);
}

void Config::debug_print() {
	const char *colors[] = {rgba(235, 198, 198, 1), rgba(168, 209, 221, 1),
				rgba(201, 184, 208, 1), rgba(179, 154, 189, 1)};
	for (size_t i = 0; i < _servers.size(); ++i)
		debug_print_server(_servers[i], colors[i % 4]);
}