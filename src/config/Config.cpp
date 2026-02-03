#include "Config.hpp"
#include <cstdlib>

Config::Config() : _isError(false), _ErrorMsg(""), _ErrorLine(0) {}
Config::~Config() {}

static unsigned long long size_max_ull() { return (unsigned long long)(~(size_t)0); }

void	Config::parse_global(std::vector<std::string> &tokens, GlobalConf &global, size_t line)
{
	std::string	&key = tokens[0];

	if (tokens.size() < 3 || tokens[tokens.size() - 1] != ";")
	{
		setError(line, "Invalid syntax, expected at least 'DIRECTIVE ARG ;'");
		return ;
	}
	else if (key == "root")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid root syntax, expected 'root PATH ;'");
			return ;
		}
		global.root = tokens[1];
	}
		else if (key == "index")
	{
		global.files.clear();
		for (size_t i = 1; (i + 1) < tokens.size(); ++i)
			global.files.push_back(tokens[i]);
	}
	else if (key == "error_page")
	{
		if (global.root.empty())
		{
			setError(line, "Root required for global error default to be set");
			return ;
		}
		if (tokens.size() != 4)
		{
			setError(line, "Invalid error syntax, expected 'error_page CODE PATH ;'");
			return ;
		}
		std::ifstream file((global.root + tokens[2]).c_str());
		std::string str;
		if (!file.is_open())
		{
			setError(line, "Could not open error file");
			return ;
		}
		std::ostringstream ss;
		ss << file.rdbuf();
		str = ss.str();
		unsigned long long code;
		if (!parse_ull(tokens[1], code) || code < 400 || code > 599)
		{
			setError(line, "Invalid error code");
			return;
		}
		global.error_pages[(int)code] = str;
	}
	else if (key == "client_max_body_size")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid max size syntax, expected 'client_max_body_size SIZE ;'");
			return ;
		}
		if (!is_valid_num(tokens[1]))
		{
			setError(line, "Invalid body size value, expected positive number");
			return ;
		}
		global.max_size = get_size(tokens[1]);
		if (global.max_size == 0)
		{
			setError(line, "Invalid/overflow value");
			return ;
		}
	}
	else if (key == "timeout")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid timeout syntax, expected 'timeout MS ;'");
			return ;
		}
		unsigned long long t;
		if (!is_valid_time(tokens[1]) || !parse_ull(tokens[1], t) || t <= 0 || t > size_max_ull())
		{
			setError(line, "Invalid/overflow timeout value");
			return;
		}
		global.timeout_ms = (size_t)t;
	}
	else
		setError(line, "Unknown directive");
}

void	Config::parse_server(std::vector<std::string> &tokens, ServerConf &server, size_t line)
{
	std::string	&key = tokens[0];

	if (tokens.size() < 3 || tokens[tokens.size() - 1] != ";")
	{
		setError(line, "Invalid syntax, expected at least 'DIRECTIVE ARG ;'");
		return ;
	}
	else if (key == "listen")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid listen syntax, expected 'Listen IP:PORT ;'");
			return ;
		}
		std::string host_port = tokens[1];
		size_t	pos = host_port.find(':');
		if (pos == std::string::npos)
		{
			uint16_t p;
			if (!parse_port(host_port, p))
			{
				setError(line, "Invalid port");
				return;
			}
			server.hosts.host_str = "0.0.0.0";
			server.hosts.host = INADDR_ANY;
			server.hosts.port_int = (int)p;
			server.hosts.port = htons(p);
		}
		else
		{
			server.hosts.host_str = host_port.substr(0, pos);
			if (!IP_to_long(server.hosts.host_str.c_str(), server.hosts.host))
			{
				setError(line, "Invalid IP");
				return;
			}
			uint16_t p;
			if (!parse_port(host_port.substr(pos + 1), p))
			{
				setError(line, "Invalid port");
				return;
			}
			server.hosts.port_int = (int)p;
			server.hosts.port = htons(p);
		}
		if (_isError)
			return ;
	}
	else if (key == "server_name")
	{
		server.names.clear();
		for (size_t i = 1; (i + 1) < tokens.size(); ++i)
			server.names.push_back(tokens[i]);
	}
	else if (key == "root")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid root syntax, expected 'root PATH ;'");
			return ;
		}
		server.root = tokens[1];
	}
		else if (key == "index")
	{
		server.files.clear();
		for (size_t i = 1; (i + 1) < tokens.size(); ++i)
			server.files.push_back(tokens[i]);
	}
	else if (key == "error_page")
	{
		if (tokens.size() != 4)
		{
			setError(line, "Invalid error syntax, expected 'error_page CODE PATH ;'");
			return ;
		}
		std::string root;
		if (server.root.empty())
		{
			if (_globalconf.root.empty())
			{
				setError(line, "No root found");
				return ;
			}
			root = _globalconf.root;
		}
		else
			root = server.root;
		std::ifstream file((root + tokens[2]).c_str());
		std::string str;
		if (!file.is_open())
		{
			setError(line, "Could not open error file");
			return ;
		}
		std::ostringstream ss;
		ss << file.rdbuf();
		str = ss.str();
		unsigned long long code;
		if (!parse_ull(tokens[1], code) || code < 400 || code > 599)
		{
			setError(line, "Invalid error code");
			return;
		}
		server.error_pages[(int)code] = str;
	}
	else if (key == "client_max_body_size")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid max size syntax, expected 'client_max_body_size SIZE ;'");
			return ;
		}
		if (!is_valid_num(tokens[1]))
		{
			setError(line, "Invalid body size value, expected positive number");
			return ;
		}
		server.max_size = get_size(tokens[1]);
		if (server.max_size == 0)
		{
			setError(line, "Invalid/overflow value");
			return ;
		}
		server.has_max_size = true;
	}
	else if (key == "timeout")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid timeout syntax, expected 'timeout MS ;'");
			return ;
		}
		unsigned long long t;
		if (!is_valid_time(tokens[1]) || !parse_ull(tokens[1], t) || t == 0 || t > size_max_ull())
		{
			setError(line, "Invalid/overflow timeout value");
			return;
		}
		server.timeout_ms = (size_t)t;
		server.has_timeout = true;
	}
	else
		setError(line, "Unknown directive");
}

void	Config::parse_location(std::vector<std::string> &tokens, LocationConf &location, size_t line)
{
	std::string	&key = tokens[0];

	if (tokens.size() == 2 && tokens[1] == ";" && tokens[0] == "internal")
		location.internal = true;
	else if (tokens.size() < 3 || tokens[tokens.size() - 1] != ";")
	{
		setError(line, "Invalid syntax, expected at least 'DIRECTIVE ARG ;'");
		return ;
	}
	else if (key == "allowed_methods")
	{
		location.methods.clear();
		for (size_t i = 1; (i + 1) < tokens.size(); ++i)
		{
			location.methods.push_back(tokens[i]);
			if (tokens[i] != "GET" && tokens[i] != "POST" && tokens[i] != "DELETE")
			{
				setError(line, "Invalid method, allowed methods: GET, POST, DELETE");
				return ;
			}
		}
	}
	else if (key == "return")
	{
		if (tokens.size() != 4)
		{
			setError(line, "Invalid redirection syntax, expected 'return CODE URL ;'");
			return ;
		}
		unsigned long long code;
		if (!parse_ull(tokens[1], code) || code < 300 || code > 308)
		{
			setError(line, "Redirect code should be between 300-308");
			return;
		}
		location.redirect_enabled = true;
		location.redirect_status = (int)code;
		location.redirect_target = tokens[2];
	}
	else if (key == "root")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid root syntax, expected 'root PATH ;'");
			return ;
		}
		location.has_root = true;
		location.root = tokens[1];
	}
	else if (key == "index")
	{
		location.index_files.clear();
		location.has_index = true;
		for (size_t i = 1; (i + 1) < tokens.size(); ++i)
			location.index_files.push_back(tokens[i]);
	}
	else if (key == "autoindex")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid autoindex syntax, expected 'autoindex on|off ;'");
			return ;
		}
		location.autoindex_set = true;
		if (tokens[1] == "on")
			location.autoindex = true;
		else if (tokens[1] == "off")
			location.autoindex = false;
		else
			setError(line, "Invalid arg, expected 'on' or 'off'");
	}
	else if (key == "upload_store")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid upload syntax, expected 'upload_store PATH ;'");
			return ;
		}
		location.upload_enabled = true;
		location.upload_location = tokens[1];
	}
	else if (key == "cgi")
	{
		if (tokens.size() != 4)
		{
			setError(line, "Invalid cgi syntax, expected 'cgi EXT PATH ;'");
			return ;
		}
		
		std::string	ext = tokens[1];
		std::string	path = tokens[2];

		if (ext.size() < 2 || ext[0] != '.')
		{
			setError(line, "CGI extension must start with '.', path and ext must not be empty");
			return ;
		}
		location.cgi[ext] = path; // to double check
		// else if (tokens[1] == ".py")
		// {
		// 	location.has_py = true;
		// 	location.py_path = tokens[2];
		// }
		// else if (tokens[1] == ".php")
		// {
		// 	location.has_php = true;
		// 	location.php_path = tokens[2];
		// }
	}
	else if (key == "cgi_timeout")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid cgi_timeout syntax, expected 'cgi_timeout MS ;'");
			return ;
		}
		unsigned long long t;
		if (!is_valid_time(tokens[1]) || !parse_ull(tokens[1], t) || t == 0 || t > size_max_ull())
		{
			setError(line, "Invalid/overflow timeout value");
			return;
		}
		location.cgi_timeout_ms = (size_t)t;
		location.has_timeout = true;
	}
	else if (key == "cgi_max_output")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid cgi max size syntax, expected 'cgi_max_output SIZE ;'");
			return ;
		}
		if (!is_valid_num(tokens[1]))
		{
			setError(line, "Invalid body size value, expected positive number");
			return ;
		}
		location.cgi_maxOutput = get_size(tokens[1]);
		if (location.cgi_maxOutput == 0)
		{
			setError(line, "Invalid/overflow value");
			return ;
		}
		location.has_maxOutput = true;
	}
	else if (key == "client_max_body_size")
	{
		if (tokens.size() != 3)
		{
			setError(line, "Invalid max size syntax, expected 'client_max_body_size SIZE ;'");
			return ;
		}
		if (!is_valid_num(tokens[1]))
		{
			setError(line, "Invalid body size value, expected positive number");
			return ;	
		}
		location.has_max_size = true;
		location.max_size = get_size(tokens[1]);
		if (location.max_size == 0)
		{
			setError(line, "Invalid/overflow value");
			return ;
		}
	}
	else
		setError(line, "Unknown directive");
}

static bool	check_in_vec(const std::vector <int>& ports, int port)
{
	size_t	i = 0;
	while (i < ports.size())
	{
		if (port == ports[i])
			return true;
		i++;
	}
	return false;
}

static bool check_hostport(std::vector <ServerConf> servers)
{
	size_t	i = 0;
	std::vector <int> ports;
	
	while (i < servers.size())
	{
		if (check_in_vec(ports, servers[i].hosts.port_int))
			return (false);
		ports.push_back(servers[i].hosts.port_int);
		i++;
	}
	return (true);
}

bool Config::parse(const std::string &filename)
{
	ServerConf	server;
	LocationConf	location;
	bool	creating_server = false;
	bool	creating_location = false;
	_servers.clear();

	Context ctx = GLOBAL;
	std::vector<std::string> lines = read_lines(filename);

	if (lines.empty())
	{
		setError(0, "Error opening file");
		return (false);
	}

	size_t i;
	for (i = 0; i < lines.size(); i++)
    {
		if (_isError)
			break ;
        std::string line = remove_coms(lines[i]);
        line = trim(line);

        if (line.empty())
            continue;

		std::vector<std::string> tokens = tokenize(line);
        if (tokens[0] == "server")
		{
			if (ctx != GLOBAL)
			{
				setError(i + 1, "server block declared in wrong scope");
				return (false);
			}
			if (tokens.size() >= 2 && tokens[1] == "{")
			{
				ctx = SERVER;
				server = ServerConf();
				creating_server = true;
			}
			else
			{
				setError(i + 1, "missing opening brace '{' or missing tokens");
				return (false);
			}
			continue;
		}

		if (tokens[0] == "location")
		{
			if (ctx != SERVER)
			{
				setError(i + 1, "location block declared in wrong scope");
				return (false);
			}
			if (tokens.size() >= 3 && tokens[2] == "{")
			{
				std::string path = tokens[1];
				ctx = LOCATION;
				location = LocationConf();
				location.path = path;
				creating_location = true;
			}
			else
			{
				setError(i + 1, "missing opening brace '{' or missing tokens");
				return (false);
			}
			continue;
		}
		if (tokens.size() == 1 && tokens[0] == "}")
		{
			if (ctx == LOCATION)
			{
				if (creating_server && creating_location)
					server.locations.push_back(location);
				creating_location = false;
				ctx = SERVER;
			}
			else if (ctx == SERVER)
			{
				if (creating_server)
					_servers.push_back(server);
				creating_server = false;
				ctx = GLOBAL;
			}
			else
			{
				setError(i + 1, "Unexpected '}'");
				return (false);
			}
			continue;
		}

		if (ctx == SERVER)
			parse_server(tokens, server, i + 1);
		else if (ctx == LOCATION)
			parse_location(tokens, location, i + 1);
		else
			parse_global(tokens, _globalconf, i + 1);
    }
	if (ctx != GLOBAL)
	{
		setError(i + 1, "Unclosed '{'");
		return false;
	}
	if (_isError)
		return (false);
	if (!check_hostport(_servers))
	{
		setError(0, "Double use of same port detected");
		return (false);
	}
	apply_defaults();
	return (valid_config());
}
