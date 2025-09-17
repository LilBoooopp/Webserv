#pragma once
#include <string>

struct HttpResponse {
	int			status;
	std::string	reason;
	std::string	contentType;
	std::string	body;
	bool		close;
};
