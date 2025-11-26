#include "Config.hpp"


Config::Config() {}
Config::~Config() {}

std::vector<std::string> read_lines(const std::string &filename)
{
    std::vector<std::string> out;
    std::ifstream file(filename.c_str());

    if (!file.is_open())
    {
        std::cerr << "Error: could not open config file: " << filename << std::endl;
        return (out);
    }

    std::string line;
    while (std::getline(file, line))
    {
        out.push_back(line);
    }

    file.close();
    return out;	
}

std::string Config::remove_coms(std::string &line)
{
    int pos = line.find('#');
    if (pos != std::string::npos)
        return line.substr(0, pos);
    return line;
}

std::string Config::trim(std::string &line)
{
    if (line.empty())
        return "";

    size_t start = 0;
    while (start < line.size() && std::isspace(line[start]))
        start++;

    size_t end = line.size();
    while (end > start && std::isspace(line[end - 1]))
        end--;

    return line.substr(start, end - start);
}

std::string Config::separate(std::string &line)
{
	std::string res;

	for (int i = 0; i < line.size(); ++i)
	{
		char c = line[i];

		if (c == '{' || c == '}' || c == ';')
		{
			res += ' ';
			res += c;
			res += ' ';
		}
		else
			res += c;
	}
	return (res);
}

std::vector<std::string> Config::tokenize(std::string &line)
{
	std::vector<std::string> tokens;
	std::string newline = separate(line);
	std::stringstream split(newline);
	std::string token;

	while (split >> token)
		tokens.push_back(token);
	return (tokens);
}

void	Config::parse_server(std::vector<std::string> &tokens, ServerConf &server)
{
	std::string	&key = tokens[0];

	if (tokens.size() < 3 || tokens[tokens.size() - 1] != ";")
		return ;
	else if (key == "listen")
	{
		std::string host_port = tokens[1];
		HostPort	res;
		int	pos = host_port.find(':');
		if (pos == std::string::npos)
		{
			res.host = "0.0.0.0";
			res.port = std::atoi(host_port.c_str());
		}
		else
		{
			res.host = host_port.substr(0, pos);
			res.port = std::atoi(host_port.substr(pos + 1).c_str());
		}
		server.hosts.push_back(res);
	}
	else if (key == "server_name")
	{
		server.names.clear();
		for (int i = 1; (i + 1) < tokens.size(); ++i)
			server.names.push_back(tokens[i]);
	}
	else if (key == "root")
		server.root = tokens[1];
	else if (key == "index")
	{
		server.files.clear();
		for (int i = 1; (i + 1) < tokens.size(); ++i)
			server.files.push_back(tokens[i]);
	}
	else if (key == "error_page")
	{
		if (tokens.size() != 4)
        	return;
		server.error_pages[std::atoi(tokens[1].c_str())] = tokens[2];
	}
	else if (key == "client_max_body_size")
	{
		int val	= std::atoi(tokens[1].c_str());
		if (val < 0)
			return ;
		server.max_size = val;
	}

}

void	Config::parse_location(std::vector<std::string> &tokens, LocationConf &location)
{
	std::string	&key = tokens[0];

	if (tokens.size() < 3 || tokens[tokens.size() - 1] != ";")
		return ;
	else if (key == "allowed_methods")
	{
		location.methods.clear();
		for (int i = 1; (i + 1) < tokens.size(); ++i)
			location.methods.push_back(tokens[i]);
	}
	else if (key == "return")
	{
		if (tokens.size() != 4)
			return ;
		location.redirect_enabled = true;
		location.redirect_status = std::atoi(tokens[1].c_str());
		location.redirect_target = tokens[2];
	}
	else if (key == "root")
	{
		if (tokens.size() != 3)
			return ;
		location.has_root = true;
		location.root = tokens[1];
	}
	else if (key == "index")
	{
		location.index_files.clear();
		location.has_index = true;
		for (int i = 1; (i + 1) < tokens.size(); ++i)
			location.index_files.push_back(tokens[i]);
	}
	else if (key == "autoindex")
	{
		if (tokens.size() != 3)
			return ;
		location.autoindex_set = true;
		if (tokens[1] == "on")
			location.autoindex = true;
		else
			location.autoindex = false;
	}
	else if (key == "upload_store")
	{
		if (tokens.size() != 3)
			return ;
		location.upload_enabled = true;
		location.upload_location = tokens[1];
	}
	else if (key == "cgi")
	{
		if (tokens.size() != 4)
			return ;
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
			return ;
		location.has_max_size = true;
		location.max_size = val;
	}
}

Conf Config::parse(const std::string &filename)
{
	Conf	conf;
	ServerConf server;
	LocationConf location;
	bool	creating_server = false;
	bool	creating_location = false;

	Context ctx = GLOBAL;
	std::vector<std::string> lines = read_lines(filename);
	
	for (int i = 0; i < lines.size(); i++)
    {
        std::string line = remove_coms(lines[i]);
        line = trim(line);

        if (line.empty())
            continue;

		std::vector<std::string> tokens = tokenize(line);
        if (ctx == GLOBAL && tokens[0] == "server")
		{
			if (tokens.size() >= 2 && tokens[1] == "{")
			{
				std::cout << "Entered server on line " << i + 1 << "\n";
				ctx = SERVER;
				server = ServerConf();
				creating_server = true;
			}
			continue;
		}

		if (ctx == SERVER && tokens[0] == "location")
		{
			if (tokens.size() >= 3 && tokens[2] == "{")
			{
				std::string path = tokens[1];
				std::cout << "Entered location on line " << i + 1 << "\n";
				ctx = LOCATION;
				location = LocationConf();
				location.path = path;
				creating_location = true;
			}
			continue;
		}

		if (tokens.size() == 1 && tokens[0] == "}")
		{
			if (ctx == LOCATION)
			{
				std::cout << "Exit location on line " << i + 1 << "\n";
				if (creating_server && creating_location)
					server.locations.push_back(location);
				creating_location = false;
				ctx = SERVER;
			}
			else if (ctx == SERVER)
			{
				std::cout << "Exit server on line " << i + 1 << "\n";
				if (creating_server)
					conf.servers.push_back(server);
				creating_server = false;
				ctx = GLOBAL;
			}
			continue;
		}

		if (ctx == SERVER)
			parse_server(tokens, server);
		else if (ctx == LOCATION)
			parse_location(tokens, location);
    }

    return conf;
}

// int main()
// {
//     Config parser;
//     Conf config = parser.parse("test.conf");
//     return 0;
// }