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

inline static const LocationConf *findLocation(const ServerConf &conf,
                                               const std::string &reqTarget) {
  const LocationConf *best = NULL;
  size_t bestLen = 0;
  for (size_t i = 0; i < conf.locations.size(); ++i) {
    const LocationConf &loc = conf.locations[i];
    const std::string &lp = loc.path;
    if (reqTarget.compare(0, lp.size(), lp) == 0) {
      if (lp.size() > bestLen) {
        best = &loc;
        bestLen = lp.size();
      }
    }
  }
  return best;
}

inline static void split(const std::string &s, char sep,
                         std::vector<std::string> &out) {
  out.clear();
  size_t i = 0;
  while (i <= s.size()) {
    size_t j = s.find(sep, i);
    if (j == std::string::npos)
      j = s.size();
    out.push_back(s.substr(i, j - i)); // <-- FIX ICI
    i = j + 1;
  }
}

inline std::string safe_join_under_root(const std::string &root,
                                        const std::string &target) {
  // Assume target starts with '/'
  if (target == "/")
    return (root + "index.html");

  std::string targ(target);
  if (!targ.empty() && targ[0] == '/')
    targ = targ.substr(1);

  std::vector<std::string> parts;
  split(targ, '/', parts);
  std::vector<std::string> clean;
  for (size_t k = 0; k < parts.size(); ++k) {
    const std::string &p = parts[k];
    if (p.empty() || p == ".")
      continue;
    if (p == "..") {
      if (!clean.empty()) {
        clean.pop_back();
        continue;
      }
    }
    clean.push_back(p);
  }
  std::string out = root;
  if (!out.empty() && out[out.size() - 1] != '/')
    out += '/';
  for (size_t k = 0; k < clean.size(); ++k) {
    out += clean[k];
    if (k + 1 < clean.size())
      out += '/';
  }
  return (out);
}

inline std::string getExtension(const std::string &str, char separator) {
  std::string::size_type qpos = str.find(separator);
  if (qpos == std::string::npos)
    return "";
  std::string extension = str.substr(qpos + 1);
  return extension;
}
