// #pragma once
// #include <string>

// struct ServerConfig {
// 	std::string root;
// 	std::string index;
// 	size_t client_max_body_size;
// 	size_t cgi_timeout_ms;
// 	size_t cgi_maxOutput;
// 	// ServerConfig() : root("www/"), index("index.html"), client_max_body_size(1 << 20) {}
// 	ServerConfig()
// 	    : root("www_giulio/"), index("index.html"), client_max_body_size(1 << 20),
// 	      cgi_timeout_ms(5000), cgi_maxOutput(1024 * 1024 * 1024) {}
// };


#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

struct HostPort
{
	std::string host;
	int			port;
};

struct LocationConf
{
	std::string	path;
	std::vector<std::string> methods;

	bool	redirect_enabled;
	int	redirect_status;
	std::string	redirect_target;

	bool	has_root;
	std::string	root;

	bool	has_index;
	std::vector<std::string>	index_files;

	bool	autoindex_set;
	bool	autoindex;

	bool	upload_enabled;
	std::string	upload_location;

	bool	has_py;
	std::string	py_path;

	bool	has_php;
	std::string	php_path;

	bool	has_max_size;
	size_t	max_size;

    LocationConf()
        : redirect_enabled(false),
          redirect_status(0),
          has_root(false),
          has_index(false),
          autoindex_set(false),
          autoindex(false),
          upload_enabled(false),
          has_max_size(false),
          max_size(0)
    {}
};

struct ServerConf
{
	std::vector<HostPort>	hosts;
	std::vector<std::string>	names;

	std::string	root;
	std::vector<std::string>	files;

	std::map<int, std::string>	error_pages;

	size_t	max_size;

	std::vector<LocationConf>	locations;

	ServerConf() : max_size(0) {}
};

struct Conf
{
	std::vector<ServerConf> servers;
};

class Config
{
	public:
		Config();
		~Config();

		Conf parse(const std::string &filename);

	private:

		std::string remove_coms(std::string &line);
		std::string trim(std::string &line);

		std::string	separate(std::string &line);
		std::vector<std::string> tokenize(std::string &line);

		void	parse_server(std::vector<std::string> &tokens, ServerConf &server);
		void	parse_location(std::vector<std::string> &tokens, LocationConf &location);

		enum Context
		{
			GLOBAL,
			SERVER,
			LOCATION
		};

};