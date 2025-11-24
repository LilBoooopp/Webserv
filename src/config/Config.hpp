#pragma once
#include <string>

struct ServerConfig {
	std::string root;
	std::string index;
	size_t client_max_body_size;
	size_t cgi_timeout_ms;
	size_t cgi_maxOutput;
	// ServerConfig() : root("www/"), index("index.html"), client_max_body_size(1 << 20) {}
	ServerConfig()
	    : root("www_giulio/"), index("index.html"), client_max_body_size(1 << 20),
	      cgi_timeout_ms(5000), cgi_maxOutput(1024 * 1024 * 1024) {}
};
