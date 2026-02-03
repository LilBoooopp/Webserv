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
	std::string	path;							// Path e.g. 'cgi/'
	std::vector<std::string> methods;			// Methods allowed in this location

	bool	redirect_enabled;					// If true, the location will only redirect
	int	redirect_status;						// The http code return
	std::string	redirect_target;				// The url that will be given for redirection

	bool	has_root;							// If false, server root will be used
	std::string	root;							// Specific root used for this location

	bool	has_index;							// if false inherited from server files
	std::vector<std::string>	index_files;	// Index files for location

	bool	autoindex_set;						// If true generate directory listing
	bool	autoindex;							// If path is directory and no index file exists an HTML directory listing should be generated

	bool	upload_enabled;						// Upload is forbidden 
	std::string	upload_location;				// If true post can upload to location

	// should be removed
	// bool has_py;
	// std::string py_path;

	// bool has_php;
	// std::string php_path;

	std::map<std::string, std::string> cgi;		// map of '.py' : 'location'  etc...
	
	size_t	cgi_timeout_ms;						// max timeout, if not set inherits from server
	bool	has_timeout;
	size_t	cgi_maxOutput;						// max output if not set inherits from location
	bool	has_maxOutput;

	bool	has_max_size;						
	size_t	max_size;							// max location output, if not set inherits from server

	bool	internal;

	LocationConf()
	    : redirect_enabled(false), redirect_status(0), has_root(false), has_index(false),
	      autoindex_set(false), autoindex(false), upload_enabled(false), 
		  //has_py(false), has_php(false), 
		  cgi_timeout_ms (1000), has_timeout(false), cgi_maxOutput (1024*1024), has_maxOutput(false),
	      has_max_size(false), max_size(1024*1024), internal(false) {}
};

struct ServerConf {
	HostPort	hosts;
	std::vector<std::string>	names;			// For virtual hosts to match server name
	
	// Not sure that this should be here...
	// bool has_py;
	// std::string py_path;
	// bool has_php;
	// std::string php_path;

	std::string	root;							// Default root for server
	std::vector<std::string>	files;			// Default index files

	std::map<int, std::string> error_pages;		// Error pages, with defaults set  'error code' : 'HTML'

	size_t	max_size;							// Default max body size'
	bool	has_max_size;
	size_t	timeout_ms;							// Default timeout
	bool	has_timeout;

	std::vector<LocationConf> locations;

	ServerConf() : has_max_size(false), has_timeout(false) {}
};

struct GlobalConf
{
	std::map<int, std::string> error_pages;		// Error pages, with defaults set  'error code' : 'HTML'

	size_t	max_size;							// Default max body size'
	size_t	timeout_ms;							// Default timeout

	std::string	root;							// Default root for server
	std::vector<std::string>	files;			// Default index files

	GlobalConf() : max_size(1024 * 1024), timeout_ms(5000) {}
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
	GlobalConf	_globalconf;

	// Config
	void	parse_global(std::vector<std::string> &tokens, GlobalConf &global, size_t line);
	void 	parse_server(std::vector<std::string> &tokens, ServerConf &server, size_t line);
	void 	parse_location(std::vector<std::string> &tokens, LocationConf &location, size_t line);

	// Config_Debug
	void debug_print_server(const ServerConf &server, const char *clr);
	void debug_print_location(const LocationConf &location, const char *clr);

	// Config_Helpers
	std::string remove_coms(std::string &line);
	std::string trim(std::string &line);
	std::string separate(std::string &line);
	std::vector<std::string> tokenize(std::string &line);
	std::vector<std::string> read_lines(const std::string &filename);
	size_t	get_size(const std::string &token);
	bool	is_valid_num(const std::string &num);
	bool	is_valid_time(const std::string &num);
	bool	parse_ull(const std::string& s, unsigned long long& res);
	bool 	parse_port(const std::string& s, uint16_t& res);

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
