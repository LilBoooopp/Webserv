#pragma once
#include <string>
#include <map>

struct Request {
	std::string	method;
	std::string	target;
	std::string	version;
	std::map<std::string, std::string> headers;
};

class Connection {
public:
	std::string	in;
	std::string	out;
	bool	headers_done;
	bool	responded;
	bool	peer_closed;
	bool	close_after;
	Connection(): headers_done(false), responded(false), peer_closed(false), close_after(false) {}
};
