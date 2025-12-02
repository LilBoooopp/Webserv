#include "Config.hpp"
#include <cstdlib>


Config::Config() : _isError(false), _ErrorMsg(""), _ErrorLine(0) {}
Config::~Config() {}

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
		std::string host_port = tokens[1];
		HostPort	res;
		size_t	pos = host_port.find(':');
		if (pos == std::string::npos)
		{
			res.host = "0.0.0.0";
			res.port = ::atoi(host_port.c_str());
		}
		else
		{
			res.host = host_port.substr(0, pos);
			res.port = ::atoi(host_port.substr(pos + 1).c_str());
		}
		server.hosts.push_back(res);
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
		server.error_pages[std::atoi(tokens[1].c_str())] = tokens[2];
	}
	else if (key == "client_max_body_size")
	{
		int val	= std::atoi(tokens[1].c_str());
		if (val < 0)
		{
			setError(line, "Invalid body size value, expected positive number");
			return ;
		}
		server.max_size = val;
	}
	else
		setError(line, "Unknown directive");

}

void	Config::parse_location(std::vector<std::string> &tokens, LocationConf &location, size_t line)
{
	std::string	&key = tokens[0];

	if (tokens.size() < 3 || tokens[tokens.size() - 1] != ";")
	{
		setError(line, "Invalid syntax, expected at least 'DIRECTIVE ARG ;'");
		return ;
	}
	else if (key == "allowed_methods")
	{
		location.methods.clear();
		for (size_t i = 1; (i + 1) < tokens.size(); ++i)
			location.methods.push_back(tokens[i]);
	}
	else if (key == "return")
	{
		if (tokens.size() != 4)
		{
			setError(line, "Invalid redirection syntax, expected 'return CODE URL ;'");
			return ;
		}
		location.redirect_enabled = true;
		location.redirect_status = std::atoi(tokens[1].c_str());
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
			setError(line, "Invalid autoindex syntax, expected 'autoindex ON|OFF ;'");
			return ;
		}
		location.autoindex_set = true;
		if (tokens[1] == "on")
			location.autoindex = true;
		else
			location.autoindex = false;
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
		else if (tokens[1] == ".py")
		{
			location.has_py = true;
			location.py_path = tokens[2];
		}
		else if (tokens[1] == ".php")
		{
			location.has_php = true;
			location.php_path = tokens[2];
		}
	}
	else if (key == "client_max_body_size")
	{
		int val	= std::atoi(tokens[1].c_str());
		if (val < 0)
		{
			setError(line, "Invalid body size value, expected positive number");
			return ;
		}
		location.has_max_size = true;
		location.max_size = val;
	}
	else
		setError(line, "Unknown directive");
}

bool Config::parse(const std::string &filename)
{
	ServerConf server;
	LocationConf location;
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
		{
			setError(i + 1, "Global directives not yet supported");
			return (false);
		}
    }
	if (ctx != GLOBAL)
	{
		setError(i + 1, "Unclosed '{'");
		return false;
	}
	return true;
}
