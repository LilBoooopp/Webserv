#pragma once
#include <string>

struct	ServerConfig {
	std::string	root;
	std::string	index;
	size_t		client_max_body_size;
	ServerConfig(): root("www"), index("index.html"), client_max_body_size(1<<20) {}
};
