#include "Config.hpp"
#include <iostream>

// Thank you chatgpt

void print_server(const ServerConf &s)
{
    std::cout << "---- SERVER ----\n";

    std::cout << "Hosts:\n";
    for (size_t i = 0; i < s.hosts.size(); ++i)
        std::cout << "  - " << s.hosts[i].host << ":" << s.hosts[i].port << "\n";

    std::cout << "Server names:";
    for (size_t i = 0; i < s.names.size(); ++i)
        std::cout << " " << s.names[i];
    std::cout << "\n";

    std::cout << "Root: " << s.root << "\n";

    std::cout << "Index files:";
    for (size_t i = 0; i < s.files.size(); ++i)
        std::cout << " " << s.files[i];
    std::cout << "\n";

    std::cout << "Error pages:\n";
    for (std::map<int,std::string>::const_iterator it = s.error_pages.begin();
         it != s.error_pages.end(); ++it)
        std::cout << "  " << it->first << " => " << it->second << "\n";

    std::cout << "Max body size: " << s.max_size << "\n";

    std::cout << "Locations: " << s.locations.size() << "\n\n";
}

void print_location(const LocationConf &loc)
{
    std::cout << "  ---- LOCATION ----\n";
    std::cout << "  Path: " << loc.path << "\n";

    std::cout << "  Methods:";
    for (size_t i = 0; i < loc.methods.size(); ++i)
        std::cout << " " << loc.methods[i];
    std::cout << "\n";

    if (loc.redirect_enabled)
        std::cout << "  Redirect: " << loc.redirect_status
                  << " -> " << loc.redirect_target << "\n";

    if (loc.has_root)
        std::cout << "  Root override: " << loc.root << "\n";

    if (loc.has_index)
    {
        std::cout << "  Index override:";
        for (size_t i = 0; i < loc.index_files.size(); ++i)
            std::cout << " " << loc.index_files[i];
        std::cout << "\n";
    }

    if (loc.autoindex_set)
        std::cout << "  Autoindex: " << (loc.autoindex ? "on" : "off") << "\n";

    if (loc.upload_enabled)
        std::cout << "  Upload store: " << loc.upload_location << "\n";

    if (loc.has_py)
        std::cout << "  CGI Python: " << loc.py_path << "\n";

    if (loc.has_php)
        std::cout << "  CGI PHP: " << loc.php_path << "\n";

    if (loc.has_max_size)
        std::cout << "  Max body size override: " << loc.max_size << "\n";
}

int main()
{
    Config parser;
    Conf conf = parser.parse("test.conf");

    std::cout << "Parsed " << conf.servers.size() << " server(s)\n\n";

    for (size_t i = 0; i < conf.servers.size(); ++i)
    {
        print_server(conf.servers[i]);

        for (size_t j = 0; j < conf.servers[i].locations.size(); ++j)
        {
            print_location(conf.servers[i].locations[j]);
            std::cout << "\n";
        }
    }

    return 0;
}