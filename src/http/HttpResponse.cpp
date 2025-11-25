#include "HttpResponse.hpp"
#include <fcntl.h>
#include <sstream>

const char *HttpResponse::reasonForStatus(int code)
{
	switch (code)
	{
		case 200: return ("OK");
		case 400: return ("Bad Request");
		case 403: return ("Forbidden");
		case 404: return ("Not Found");
		case 405: return ("Method Not Allowed");
		case 413: return ("Payload Too Large");
		case 501: return ("Not Implemented");
		case 505: return ("HTTP Version Not Supported");
		default: return ("Unknown");
	}
}

HttpResponse::HttpResponse(int statusCode, const std::string& version)
: version_(version), statusCode_(statusCode), reasonPhrase_(reasonForStatus(statusCode)), body_("")
{
	headers_["Content-Type"] = "text/plain";
	headers_["Connection"] = "close";
}

void HttpResponse::setVersion(const std::string &version)
{
	version_ = version;
}

void HttpResponse::setVersion(const std::string &version)
{
	version_ = version;
}

void HttpResponse::setStatus(int code, const std::string &reason) {
	statusCode_ = code;
	reasonPhrase_ = reason;
}

void HttpResponse::setStatusFromCode(int code)
{
	statusCode_ = code;
	reasonPhrase_ = reasonForStatus(code);
}

void HttpResponse::setStatusFromCode(int code)
{
	statusCode_ = code;
	reasonPhrase_ = reasonForStatus(code);
}

int HttpResponse::getStatus() { return statusCode_; }

void HttpResponse::setBody(const std::string &body) {
	body_ = body;
	headers_["Content-Length"] = toString(body.size());
}

void	HttpResponse::setBodyIfEmpty(const std::string &body)
{
	if (body_.empty())
	{
		body_ = body;
		headers_["Content-Length"] = toString(body_.size());
	}
}

void	HttpResponse::ensureDefaultBodyIfEmpty(void)
{
	if (!body_.empty())
		return	;

	// default HTML error page
	std::ostringstream	oss;
	oss	<< "<!DOCTYPE html>" << std::endl
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

void	HttpResponse::setBodyIfEmpty(const std::string &body)
{
	if (body_.empty())
	{
		body_ = body;
		headers_["Content-Length"] = toString(body_.size());
	}
}

void	HttpResponse::ensureDefaultBodyIfEmpty(void)
{
	if (!body_.empty())
		return	;

	// default HTML error page
	std::ostringstream	oss;
	oss	<< "<!DOCTYPE html>" << std::endl
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

void HttpResponse::setContentType(const std::string &type) { headers_["Content-Type"] = type; }

void HttpResponse::setHeader(const std::string &key, const std::string &value) {
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

	oss << version_ << " " << statusCode_ << " " << reasonPhrase_ << "\r\n";

	// Headers
	for (std::map<std::string,std::string>::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
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
