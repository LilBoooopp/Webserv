#include "HttpResponse.hpp"

const char *HttpResponse::reasonForStatus(int code) {
  switch (code) {
  case 200:
    return ("OK");
  case 201:
    return ("Created");
  case 204:
    return ("No Content");
  case 400:
    return ("Bad Request");
  case 401:
    return ("Unauthorized");
  case 403:
    return ("Forbidden");
  case 404:
    return ("Not Found");
  case 405:
    return ("Method Not Allowed");
  case 413:
    return ("Payload Too Large");
  case 501:
    return ("Not Implemented");
  case 502:
    return ("Bad Gateway");
  case 505:
    return ("HTTP Version Not Supported");
  default:
    return ("Unknown");
  }
}

HttpResponse::HttpResponse(int statusCode, const std::string &version)
    : version_(version), statusCode_(statusCode),
      reasonPhrase_(reasonForStatus(statusCode)), body_("") {
  headers_["Content-Type"] = "text/plain";
}

void HttpResponse::setVersion(const std::string &version) {
  version_ = version;
}

void HttpResponse::setStatus(int code, const std::string &reason) {
  statusCode_ = code;
  reasonPhrase_ = reason;
}

void HttpResponse::setStatusFromCode(int code) {
  statusCode_ = code;
  reasonPhrase_ = reasonForStatus(code);
}

int HttpResponse::getStatus() { return statusCode_; }

void HttpResponse::setBody(const std::string &body) {
  body_ = body;
  headers_["Content-Length"] = toString(body.size());
}

void HttpResponse::setBodyIfEmpty(const std::string &body) {
  if (body_.empty()) {
    body_ = body;
    headers_["Content-Length"] = toString(body_.size());
  }
}

void HttpResponse::ensureDefaultBodyIfEmpty(void) {
  if (!body_.empty())
    return;

  // default HTML error page
  std::ostringstream oss;
  oss << "<!DOCTYPE html>" << std::endl
      << "<html><head><title>" << statusCode_ << " " << reasonPhrase_
      << "</title></head><body>" << std::endl
      << "<h1>" << statusCode_ << " " << reasonPhrase_ << "</h1>" << std::endl
      << "<p> The server encountered an error.</p>" << std::endl
      << "</body></html>" << std::endl;

  body_ = oss.str();

  // Default body is HTML
  headers_["Content-Type"] = "text/html";
  headers_["Content-Length"] = toString(body_.size());
}

void HttpResponse::setContentType(const std::string &type) {
  setHeader("Content-Type", type);
}

void HttpResponse::setHeader(const std::string &key, const std::string &value) {
  headers_[key] = value;
}

bool HttpResponse::hasHeader(const std::string &key) const {
  std::map<std::string, std::string>::const_iterator it = headers_.find(key);
  return (it != headers_.end());
}

std::string HttpResponse::getHeader(const std::string &key) const {
  std::map<std::string, std::string>::const_iterator it = headers_.find(key);
  if (it != headers_.end())
    return (it->second);
  return std::string();
}

void HttpResponse::eraseHeader(const std::string &key) { headers_.erase(key); }

std::string HttpResponse::getHead() const {
  std::ostringstream oss;
  oss << version_ << " " << statusCode_ << " " << reasonPhrase_;
  return oss.str();
}

std::string HttpResponse::serialize(bool headOnly) const {
  std::ostringstream oss;

  oss << version_ << " " << statusCode_ << " " << reasonPhrase_ << "\r\n";

  // Headers
  for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
       it != headers_.end(); ++it)
    oss << it->first << ": " << it->second << "\r\n";

  // Blank line separates head andbody
  oss << "\r\n";

  if (!headOnly)
    oss << body_;

  return (oss.str());
}

std::string HttpResponse::toString(size_t n) {
  std::ostringstream oss;
  oss << n;
  return (oss.str());
}

void HttpResponse::printResponse(int fd) {
  if (!Logger::channels[LOG_RESPONSE])
    return;
  std::string contentType = this->getHeader("Content-Type");
  const char *clr = rgba(146, 122, 152, 1);
  std::string label;
  if (!body_.empty() && getStatus() == 200) {
    std::ostringstream oss;
    oss << " - " << contentType << " - '" << VALUECLR << body_.substr(0, 12)
        << GREY;
    if (body_.length() > 12)
      oss << "...(" << body_.length() << ")";
    oss << "'";
    label = oss.str();
  }
  Logger::response("at fd %d: %s%s%s %s", fd, getStatus() < 400 ? GREEN : RED,
                   getHead().c_str(), TS, label.c_str());
  for (std::map<std::string, std::string>::const_iterator it =
           this->headers_.begin();
       it != this->headers_.end(); it++) {
    Logger::header("%s%-20s < %s", clr, it->first.c_str(), it->second.c_str());
  }
  if (Logger::channels[LOG_BODY]) {
    std::string preview = this->body_.substr(0, 100).c_str();
    Logger::simple("%sBODY (%d of %d) - \n\'%s...\'", clr, preview.size(),
                   this->body_.size(), preview.c_str());
  }
  Logger::simple("");
}
