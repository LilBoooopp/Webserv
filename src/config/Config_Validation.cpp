#include "Config.hpp"
#include <cstdio>

bool Config::IP_to_long(const char *addr, uint32_t &res) {
  unsigned int a, b, c, d;
  char extra;

  if (std::sscanf(addr, "%u.%u.%u.%u %c", &a, &b, &c, &d, &extra) != 4)
    return (false);
  if (a > 255 || b > 255 || c > 255 || d > 255)
    return (false);
  res = ((a & 0xFF)) | ((b & 0xFF) << 8) | ((c & 0xFF) << 16) |
        ((d & 0xFF) << 24);
  return (true);
}

bool Config::is_num(std::string str) {
  for (size_t i = 0; i < str.size(); ++i) {
    if (!isdigit(str[i]))
      return (false);
  }
  return (true);
}

void Config::apply_defaults() {
  for (size_t i = 0; i < _servers.size(); ++i) {
    if (_servers[i].files.empty())
      _servers[i].files.push_back("index.html");
    for (size_t j = 0; j < _servers[i].locations.size(); ++j) {
      if (!_servers[i].locations[j].has_root)
        _servers[i].locations[j].root = _servers[i].root;
      if (!_servers[i].locations[j].has_index)
        _servers[i].locations[j].index_files = _servers[i].files;
      if (!_servers[i].locations[j].has_max_size)
        _servers[i].locations[j].max_size = _servers[i].max_size;

      if (_servers[i].locations[j].methods.empty())
        _servers[i].locations[j].methods.push_back("GET");
    }
  }
}

bool Config::valid_config() {
  for (size_t i = 0; i < _servers.size(); ++i) {
    if (_servers[i].hosts.empty())
      setError(-1, "Missing host in server");
    if (_servers[i].root.empty())
      setError(-1, "Missing root in server");
    if (_isError)
      return (false);
  }
  return (true);
}
