#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

bool	Config::is_valid_num(const std::string &num)
{
	int	end = num.size();
	int	i = 0;
	int	dec_count = 0;

	if (num.empty())
		return (false);
	while (i < end - 1)
	{
		if (num[i] == '.' && dec_count == 0)
			dec_count += 1;
		else if (!isdigit(num[i]))
			return (false);
		i++;
	}
	if (dec_count == 1)
	{
		int pos = num.find('.');
		if (!isdigit(num[pos + 1]) || isdigit(num[i]))
			return (false);
	}
	if (isdigit(num[i]))
		return(true);
	char final_char = std::tolower(num[i]);
	if (final_char != 'k' && final_char != 'm' && final_char != 'g' && final_char != 't')
		return(false);
	if (dec_count == 0)
		return (true);
	if (final_char == 'k')
	{
		if (num.size() - num.find('.') - 2 > 3)
			return (false);
	}
	if (final_char == 'm')
	{
		if (num.size() - num.find('.') - 2 > 6)
			return (false);
	}
	if (final_char == 'g')
	{
		if (num.size() - num.find('.') - 2 > 9)
			return (false);
	}
	if (final_char == 't')
	{
		if (num.size() - num.find('.') - 2 > 12)
			return (false);
	}
	return (true);
}

size_t  Config::get_size(const std::string &token)
{
	int end = token.size();
	if (isdigit(token[end - 1]))
		return std::atoi(token.c_str());

	char suf = std::tolower(token[end - 1]);
	std::string s_num = token.substr(0, token.size() - 1);
	std::stringstream ss(s_num);
	double	num;
	ss >> num;

	if (suf == 'k')
		return (num * 1000);
	if (suf == 'm')
		return (num * 1000000);
	if (suf == 'g')
		return (num * 1000000000);
	if (suf == 't')
		return (num * 1000000000000);
	return (0);
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

