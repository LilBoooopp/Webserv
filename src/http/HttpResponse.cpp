#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse(int statusCode, const std::string& reason)
: statusCode_(statusCode), reasonPhrase_(reason), body_("")
{
	headers_["Content-Type"] = "text/plain";
	headers_["Connection"] = "close";
}

void	HttpResponse::setStatus(int code, const std::string& reason)
{
	statusCode_ = code;
	reasonPhrase_ = reason;
}

void	HttpResponse::setBody(const std::string& body)
{
	body_ = body;
	headers_["Content-Length"] = toString(body.size());
}

void	HttpResponse::setContentType(const std::string& type)
{
	headers_["Content-Type"] = type;
}

void	HttpResponse::setHeader(const std::string& key, const std::string& value)
{
	headers_[key] = value;
}

bool	HttpResponse::hasHeader(const std::string& key) const
{
	std::map<std::string,std::string>::const_iterator it = headers_.find(key);
	return (it != headers_.end());
}

std::string HttpResponse::getHeader(const std::string& key) const
{
	std::map<std::string,std::string>::const_iterator it = headers_.find(key);
	if (it != headers_.end())
		return (it->second);
	return std::string();
}

void	HttpResponse::eraseHeader(const std::string& key)
{
	headers_.erase(key);
}

std::string	HttpResponse::serialize(bool headOnly) const {
	std::ostringstream	oss;
	oss << "HTTP/1.1 " << statusCode_ << " " << reasonPhrase_ << "\r\n";
	for (std::map<std::string,std::string>::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";
	oss << "\r\n";
	if (!headOnly)
		oss << body_;

	return (oss.str());
}

std::string	HttpResponse::toString(size_t n)
{
	std::ostringstream	oss;
	oss << n;
	return (oss.str());
}
