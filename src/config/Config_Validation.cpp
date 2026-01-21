#include "Config.hpp"

bool Config::IP_to_long(const char *addr, uint32_t &res) {
	unsigned int a, b, c, d;
	char extra;

	if (std::sscanf(addr, "%u.%u.%u.%u %c", &a, &b, &c, &d, &extra) != 4)
		return (false);
	if (a > 255 || b > 255 || c > 255 || d > 255)
		return (false);
	res = ((a & 0xFF)) | ((b & 0xFF) << 8) | ((c & 0xFF) << 16) | ((d & 0xFF) << 24);
	return (true);
}

bool Config::is_num(std::string str) {
	if (str.empty())
		return false;
	for (size_t i = 0; i < str.size(); ++i) {
		if (!isdigit((unsigned char)str[i]))
			return (false);
	}
	return (true);
}

static const char *reasonForStatus(int code) {
	switch (code) {
	case 400:
		return ("Bad Request");
	case 403:
		return ("Forbidden");
	case 404:
		return ("Not Found");
	case 405:
		return ("Method Not Allowed");
	case 413:
		return ("Payload Too Large");
	case 501:
		return ("Not Implemented");
	case 505:
		return ("HTTP Version Not Supported");
	default:
		return ("Unknown");
	}
}

static std::string build_default_error_page(int code) {
	const char *reason = reasonForStatus(code);

	std::ostringstream oss;
	oss << "<!DOCTYPE html>\n"
	    << "<html><head><title>" << code << " " << reason << "</title></head><body>\n"
	    << "<h1>" << code << " " << reason << "</h1>\n"
	    << "<p>The server encountered an error.</p>\n"
	    << "</body></html>\n";

	return oss.str();
}

void Config::apply_defaults() {
	int codes[] = {400, 403, 404, 405, 413, 501, 505};

	for (size_t i = 0; i < _servers.size(); ++i) {
		if (_servers[i].hosts.empty())
			setError(0, "Missing host in server");
		if (_servers[i].root.empty())
		{
			if (_globalconf.root.empty())
				setError(0, "Missing root in server");
			else
				_servers[i].root = _globalconf.root;
		}
		if (_isError)
			return ;
		if (_servers[i].files.empty())
		{
			if (_globalconf.files.empty())
				_servers[i].files.push_back("index.html");
			else
				_servers[i].files = _globalconf.files;
		}
		if (!_servers[i].has_max_size)
			_servers[i].max_size = _globalconf.max_size;
		if (!_servers[i].has_timeout)
			_servers[i].timeout_ms = _globalconf.timeout_ms;
		for (size_t k = 0; k < 7; ++k) {
			if (_servers[i].error_pages.find(codes[k]) == _servers[i].error_pages.end())
			{
				if (_globalconf.error_pages.find(codes[k]) != _globalconf.error_pages.end())
					_servers[i].error_pages[codes[k]] = _globalconf.error_pages[codes[k]];
				else
					_servers[i].error_pages[codes[k]] =
						build_default_error_page(codes[k]);
			}
		}
		if (_servers[i].locations.size() == 0) {
			LocationConf location = LocationConf();
			location.path = "/";
			_servers[i].locations.push_back(location);
		}
		for (size_t j = 0; j < _servers[i].locations.size(); ++j) {
			if (!_servers[i].locations[j].has_root)
				_servers[i].locations[j].root = _servers[i].root;
			if (!_servers[i].locations[j].has_index)
				_servers[i].locations[j].index_files = _servers[i].files;
			if (!_servers[i].locations[j].has_max_size)
				_servers[i].locations[j].max_size = _servers[i].max_size;
			if (!_servers[i].locations[j].has_maxOutput)
				_servers[i].locations[j].cgi_maxOutput =
				    _servers[i].locations[j].max_size;
			if (!_servers[i].locations[j].has_timeout)
				_servers[i].locations[j].cgi_timeout_ms =
				    _servers[i].timeout_ms;
			if (_servers[i].locations[j].methods.empty())
				_servers[i].locations[j].methods.push_back("GET");
		}
	}
}

bool Config::valid_config() {
	if (_isError)
		return (false);
	return (true);
}
