#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "Config.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

void Config::debug_print_location(const LocationConf &location, const char *clr) {
	std::cout << "        " << TS << location.path << std::endl;
	std::cout << clr << "            Methods:" << GREY;
	for (size_t i = 0; i < location.methods.size(); ++i)
		std::cout << " " << location.methods[i];
	std::cout << std::endl;

	if (location.redirect_enabled)
		std::cout << clr << "            Redirect: " << GREY << location.redirect_status
			  << " " << location.redirect_target << std::endl;
	if (location.has_root)
		std::cout << clr << "            Root override: " << GREY << location.root
			  << std::endl;
	if (location.has_index) {
		std::cout << clr << "            Index override:" << GREY;
		for (size_t i = 0; i < location.index_files.size(); ++i)
			std::cout << " " << location.index_files[i];
		std::cout << std::endl;
	}
	if (location.autoindex_set)
		std::cout << clr << "             Autoindex: " << GREY
			  << (location.autoindex ? "on" : "off") << std::endl;
	if (location.upload_enabled)
		std::cout << clr << "             Uploads: " << GREY << location.upload_location
			  << std::endl;
	if (location.has_py)
		std::cout << clr << "             .py: " << GREY << location.py_path << std::endl;
	if (location.has_php)
		std::cout << clr << "             .php: " << GREY << location.php_path << std::endl;
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
	std::cout << "    Hosts:" << GREY << std::endl;
	for (size_t i = 0; i < server.hosts.size(); ++i)
		std::cout << "	" << server.hosts[i].host_str << ":" << server.hosts[i].port_int
			  << std::endl;
	std::cout << clr << "    Names:" << GREY;
	for (size_t i = 0; i < server.names.size(); ++i)
		std::cout << " " << server.names[i];
	std::cout << std::endl;
	std::cout << clr << "    Root: " << GREY << server.root << std::endl;
	std::cout << clr << "    Index:" << GREY;
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
	std::cout << TS << std::endl;
}

void Config::debug_print() {
	const char *colors[] = {rgba(235, 198, 198, 1), rgba(168, 209, 221, 1),
				rgba(201, 184, 208, 1), rgba(179, 154, 189, 1)};
	for (size_t i = 0; i < _servers.size(); ++i) {
		std::cout << colors[i % 4] << "CFG " << i << std::endl;
		debug_print_server(_servers[i], colors[i % 4]);
	}
}
