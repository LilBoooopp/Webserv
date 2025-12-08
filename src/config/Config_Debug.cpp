#include "Config.hpp"
#include <iostream>

void Config::debug_print_location(const LocationConf &location) {
  std::cout << "        Location: " << location.path << std::endl;
  std::cout << "            Methods:";
  for (size_t i = 0; i < location.methods.size(); ++i)
    std::cout << " " << location.methods[i];
  std::cout << std::endl;

  if (location.redirect_enabled)
    std::cout << "            Redirect: " << location.redirect_status << " "
              << location.redirect_target << std::endl;
  if (location.has_root)
    std::cout << "            Root override: " << location.root << std::endl;
  if (location.has_index) {
    std::cout << "            Index override:";
    for (size_t i = 0; i < location.index_files.size(); ++i)
      std::cout << " " << location.index_files[i];
    std::cout << std::endl;
  }
  if (location.autoindex_set)
    std::cout << "             Autoindex: "
              << (location.autoindex ? "on" : "off") << std::endl;
  if (location.upload_enabled)
    std::cout << "             Uploads: " << location.upload_location
              << std::endl;
  if (location.has_py)
    std::cout << "             .py: " << location.py_path << std::endl;
  if (location.has_php)
    std::cout << "             .php: " << location.php_path << std::endl;
  if (location.has_max_size)
    std::cout << "            Max Size override: " << location.max_size
              << std::endl;
}

void Config::debug_print_server(const ServerConf &server) {
  std::cout << "Server:" << std::endl;
  std::cout << "    Hosts:" << std::endl;
  for (size_t i = 0; i < server.hosts.size(); ++i)
    std::cout << " " << server.hosts[i].host << ":" << server.hosts[i].port
              << std::endl;
  std::cout << "    Names:";
  for (size_t i = 0; i < server.names.size(); ++i)
    std::cout << " " << server.names[i];
  std::cout << std::endl;
  std::cout << "    Root: " << server.root << std::endl;
  std::cout << "    Index:";
  for (size_t i = 0; i < server.files.size(); ++i)
    std::cout << " " << server.files[i];
  std::cout << std::endl;
  std::cout << "    Error pages:" << std::endl;
  for (std::map<int, std::string>::const_iterator it =
           server.error_pages.begin();
       it != server.error_pages.end(); ++it)
    std::cout << "        " << it->first << " " << it->second;
  std::cout << std::endl;
  std::cout << "    Max body size: " << server.max_size << std::endl;
  std::cout << "    Locations:" << std::endl;
  for (size_t i = 0; i < server.locations.size(); ++i)
    debug_print_location(server.locations[i]);
}

void Config::debug_print() {
  for (size_t i = 0; i < _servers.size(); ++i)
    debug_print_server(_servers[i]);
}
