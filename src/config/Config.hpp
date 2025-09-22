#pragma once
#include <string>

struct	ServerConfig {
	std::string	root;
	std::string	index;
	ServerConfig(): root("www"), index("index.html") {}
};
