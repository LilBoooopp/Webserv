#pragma once
#include <string>

inline std::string	mime_from_path(const std::string& p) {
	size_t	dot = p.rfind('.');
	std::string	ext = (dot == std::string::npos) ? "" : p.substr(dot + 1);
	if (ext == "html" || ext == "htm")
		return ("text/html");
	if (ext == "css")
		return	("text/css");
	if (ext == "js")
		return ("application/javascript");
	if (ext == "png")
		return ("image/png");
	if (ext == "jpg" || ext == "jpeg")
		return ("image/jpeg");
	if (ext == "gif")
		return ("image/gif");
	if (ext == "svg")
		return ("image/svg+xml");
	if (ext == "txt")
		return ("text/plain");
	return ("application/octet-stream");
}
	