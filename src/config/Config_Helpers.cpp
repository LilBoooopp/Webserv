#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

bool	Config::parse_ull(const std::string& s, unsigned long long& res)
{
	errno = 0;
	char* end = 0;
	res = std::strtoull(s.c_str(), &end, 10);
	if (errno == ERANGE || end == s.c_str() || *end != '\0')
		return false;
	return true;
}

bool Config::parse_port(const std::string& s, uint16_t& res)
{
    unsigned long long val;
    if (!parse_ull(s, val))
		return false;
    if (val < 1 || val > 65535)
		return false;
    res = (uint16_t)val;
    return true;
}

bool	Config::is_valid_num(const std::string &num)
{
	int	end = num.size();
	int	i = 0;

	if (num.empty())
		return (false);
	while (i < end - 1)
	{
		if (!isdigit((unsigned char) num[i]))
			return (false);
		i++;
	}
	if (isdigit((unsigned char) num[i]))
		return(true);
	if (end == 1)
		return (false);
	char final_char = std::tolower((unsigned char) num[i]);
	if (final_char != 'k' && final_char != 'm' && final_char != 'g' && final_char != 't')
		return(false);

	return (true);
}

bool	Config::is_valid_time(const std::string &num)
{
	int	end = num.size();
	int	i = 0;

	if (num.empty())
		return (false);
	while (i < end)
	{
		if (!isdigit((unsigned char) num[i]))
			return (false);
		i++;
	}
	return (true);
}

static size_t size_max_value() { return (size_t)~(size_t)0; }

size_t Config::get_size(const std::string &token)
{
    if (token.empty())
        return 0;

    unsigned long long mult = 1;
    std::string numpart = token;

    unsigned char last_uc = (unsigned char)token[token.size() - 1];
    if (std::isalpha(last_uc))
    {
        char suf = (char)std::tolower(last_uc);
        if (suf == 'k') mult = 1000ULL;
        else if (suf == 'm') mult = 1000000ULL;
        else if (suf == 'g') mult = 1000000000ULL;
        else if (suf == 't') mult = 1000000000000ULL;
        else return 0;

        numpart = token.substr(0, token.size() - 1);
        if (numpart.empty())
            return 0;
    }

    errno = 0;
    char *endptr = 0;
    unsigned long long v = std::strtoull(numpart.c_str(), &endptr, 10);

    if (errno == ERANGE || endptr == numpart.c_str() || *endptr != '\0')
        return 0;

    unsigned long long szmax = (unsigned long long)size_max_value();
    if (v > szmax / mult)
        return 0;

    return (size_t)(v * mult);
}


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
  while (start < line.size() && std::isspace((unsigned char)line[start]))
    start++;

  size_t end = line.size();
  while (end > start && std::isspace((unsigned char)line[end - 1]))
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

