#pragma once
#include "../config/Config.hpp"
#include <sstream>
#include <string>
#include <vector>

template <typename T> std::string to_string(const T &value) {
  std::stringstream ss;
  ss << value;
  return (ss.str());
}

static std::vector<std::string> split(const std::string &s, char sep) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, sep)) {
    tokens.push_back(token);
  }
  return (tokens);
}

static std::string sanitize_path(const std::string &path) {
  std::vector<std::string> parts = split(path, '/');
  std::vector<std::string> stack;

  for (size_t i = 0; i < parts.size(); ++i) {
    if (parts[i] == ".." && !stack.empty()) {
      stack.pop_back();
    } else if (parts[i] != "." && parts[i] != ".." && !parts[i].empty()) {
      stack.push_back(parts[i]);
    }
  }

  std::string result = "";
  for (size_t i = 0; i < stack.size(); ++i) {
    result += "/" + stack[i];
  }

  return (result.empty() ? "/" : result);
}

inline std::string safe_join_under_root(const std::string &root,
                                        const std::string &target) {
  std::string clean_req = sanitize_path(target);

  std::string clean_root = root;
  if (!clean_root.empty() && clean_root[clean_root.size() - 1] == '/') {
    clean_root.erase(clean_root.size() - 1);
  }
  return (clean_root + clean_req);
}

inline std::string getExtension(const std::string &str, char separator) {
  std::string::size_type qpos = str.find(separator);
  if (qpos == std::string::npos)
    return "";
  std::string extension = str.substr(qpos + 1);
  return extension;
}
