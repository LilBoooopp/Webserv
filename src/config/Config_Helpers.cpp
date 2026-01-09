#include "Config.hpp"
#include <fstream>
#include <sstream>

std::vector<std::string> Config::read_lines(const std::string &filename) {
  std::vector<std::string> out;
  std::ifstream file(filename.c_str());

  if (!file.is_open())
    return (out);

  std::string line;
  while (std::getline(file, line)) {
    out.push_back(line);
  }

  file.close();
  return out;
}

std::string Config::remove_coms(std::string &line) {
  size_t pos = line.find('#');
  if (pos != std::string::npos)
    return line.substr(0, pos);
  return line;
}

std::string Config::trim(std::string &line) {
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

std::string Config::separate(std::string &line) {
  std::string res;

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];

    if (c == '{' || c == '}' || c == ';') {
      res += ' ';
      res += c;
      res += ' ';
    } else
      res += c;
  }
  return (res);
}

std::vector<std::string> Config::tokenize(std::string &line) {
  std::vector<std::string> tokens;
  std::string newline = separate(line);
  std::stringstream split(newline);
  std::string token;

  while (split >> token)
    tokens.push_back(token);
  return (tokens);
}

