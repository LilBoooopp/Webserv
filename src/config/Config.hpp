#pragma once
#include <string>

struct Timeouts
{
	int			client_header_timeout_ms;	// inactivity while reading headers
	int			client_body_timeout_ms;		// inactivity while reading body
	int			send_timeout_ms;			// inactivity while sending response
	int			keepalive_timeout_ms;		// idle keep-alive timeout
};
struct	ServerConfig {
	std::string	root;
	std::string	index;
	size_t		client_max_body_size;
	Timeouts	timeouts;
	ServerConfig(): root("www"), index("index.html"), client_max_body_size(1<<20)
	{
		timeouts.client_header_timeout_ms= 1500;
		timeouts.client_body_timeout_ms = 3000;
		timeouts.send_timeout_ms = 3000;
		timeouts.keepalive_timeout_ms = 5000;
	}
};
