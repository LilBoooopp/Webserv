#include "HttpParser.hpp"

#include <cctype>
#include <map>
#include <string>

static inline std::string to_lower(const std::string &s) {
  std::string r = s;
  for (size_t i = 0; i < r.size(); ++i)
    r[i] = (char)std::tolower((unsigned char)r[i]);
  return (r);
}

static inline void rstrip_cr(std::string &s) {
  if (!s.empty() && s[s.size() - 1] == '\r')
    s.erase(s.size() - 1);
}

/**
 * @brief Finds and saves "Method" (e.g. GET, POST, etc) "Target", and "version"
 * (e.g. HTTP/1.1)
 * @param line
 * @param req
 * @return Bool checking version == "HTTP/1.1"
 */
static bool parse_start_line(const std::string &line, HttpRequest &req) {
  size_t p1 = line.find(' ');
  if (p1 == std::string::npos)
    return (false);
  size_t p2 = line.find(' ', p1 + 1);
  if (p2 == std::string::npos)
    return (false);

  req.method = line.substr(0, p1);
  req.target = line.substr(p1 + 1, p2 - (p1 + 1));
  req.version = line.substr(p2 + 1);

  return (req.version == "HTTP/1.1" || req.version == "HTTP/1.0");
}

static bool parse_headers_block(const std::string &block, HttpRequest &req) {
  size_t start = 0;

  while (start < block.size()) {
    size_t end = block.find("\r\n", start);
    std::string line = (end == std::string::npos)
                           ? block.substr(start)
                           : block.substr(start, end - start);

    rstrip_cr(line);
    if (line.empty())
      break;

    size_t colon = line.find(':');
    if (colon == std::string::npos)
      return (false);

    std::string key = to_lower(line.substr(0, colon));
    size_t v = colon + 1;
    while (v < line.size() && (line[v] == ' ' || line[v] == '\t'))
      ++v;
    std::string val = line.substr(v);

    req.headers[key] = val;

    if (end == std::string::npos)
      break;
    start = end + 2;
  }

  return (true);
}

// public API
bool HttpParser::parse(Connection &c, size_t &endpos) {
  // Find end of head (CRLF CRLF)
  size_t sep = c.in.find("\r\n\r\n");
  if (sep == std::string::npos)
    return (false);

  // First line
  size_t eol = c.in.find("\r\n");
  if (eol == std::string::npos)
    return (false);

  std::string start_line = c.in.substr(0, eol);
  rstrip_cr(start_line);
  if (!parse_start_line(start_line, c.req))
    return (false);

  // Headers (no trailing blank line here)
  std::string headers;
  if (eol + 2 < sep)
    headers = c.in.substr(eol + 2, sep - (eol + 2));
  else
    headers.clear();

  if (!parse_headers_block(headers, c.req))
    return (false);

  endpos = sep + 4;
  return (true);
}
