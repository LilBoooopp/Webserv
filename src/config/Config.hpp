// #pragma once
// #include <string>

// struct ServerConfig {
// 	std::string root;
// 	std::string index;
// 	size_t client_max_body_size;
// 	size_t cgi_timeout_ms;
// 	size_t cgi_maxOutput;
// 	// ServerConfig() : root("www/"), index("index.html"),
// client_max_body_size(1 << 20) {} 	ServerConfig() 	    :
// root("www_giulio/"), index("index.html"), client_max_body_size(1 << 20),
// cgi_timeout_ms(5000), cgi_maxOutput(1024 * 1024 * 1024) {}
// };

#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <aio.h>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct HostPort {
	std::string host_str;
	uint32_t host;
	int port_int;
	uint16_t port;
};

struct LocationConf {
	std::string path;
	std::vector<std::string> methods;
	std::map<std::string, std::string> cgi;

	bool redirect_enabled;
	int redirect_status;
	std::string redirect_target;

	bool has_root;
	std::string root;

	bool has_index;
	std::vector<std::string> index_files;

	bool autoindex_set;
	bool autoindex;

	bool upload_enabled;
	std::string upload_location;

	bool has_py;
	std::string py_path;

	bool has_php;
	std::string php_path;

	size_t cgi_timeout_ms;
	size_t cgi_maxOutput;

	bool has_max_size;
	size_t max_size;

	LocationConf()
	    : redirect_enabled(false), redirect_status(0), has_root(false), has_index(false),
	      autoindex_set(false), autoindex(false), upload_enabled(false), has_py(false),
	      has_php(false), cgi_timeout_ms(10000), cgi_maxOutput(1024 * 1024),
	      has_max_size(false), max_size(1024 * 1024) {}
};

struct ServerConf {
	std::vector<HostPort> hosts;
	std::vector<std::string> names;
	bool has_py;
	std::string py_path;
	bool has_php;
	std::string php_path;

	std::string root;
	std::vector<std::string> files;

	std::map<int, std::string> error_pages;

	size_t max_size;
	size_t timeout_ms;

	std::vector<LocationConf> locations;

	ServerConf() : max_size(1024 * 1024), timeout_ms(5000) {}
};

class Config {
    public:
	Config();
	~Config();

	bool parse(const std::string &filename);
	bool hasError() const { return _isError; }
	const std::string &getErrorMessage() const { return _ErrorMsg; }
	size_t getErrorLine() const { return _ErrorLine; }
	const std::vector<ServerConf> &getServers() const { return _servers; }
	void debug_print();

    private:
	std::vector<ServerConf> _servers;

	// Config
	void parse_server(std::vector<std::string> &tokens, ServerConf &server, size_t line);
	void parse_location(std::vector<std::string> &tokens, LocationConf &location, size_t line);

	// Config_Debug
	void debug_print_server(const ServerConf &server, const char *clr);
	void debug_print_location(const LocationConf &location, const char *clr);

	// Config_Helpers
	std::string remove_coms(std::string &line);
	std::string trim(std::string &line);
	std::string separate(std::string &line);
	std::vector<std::string> tokenize(std::string &line);
	std::vector<std::string> read_lines(const std::string &filename);

	// Config_Error
	bool _isError;
	std::string _ErrorMsg;
	size_t _ErrorLine;
	void setError(size_t line, const std::string &msg);

	// Config_Validation
	bool valid_config();
	bool IP_to_long(const char *addr, uint32_t &res);
	bool is_num(std::string str);
	void apply_defaults();

	enum Context { GLOBAL, SERVER, LOCATION };
};

#endif
