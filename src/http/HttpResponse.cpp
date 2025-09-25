#include "HttpResponse.hpp"

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


// std::string	HttpResponse::toWire(bool head_only) const
// {
// 	const size_t	content_len = body.size();

// 	char	start[256];
// 	int		n = std::snprintf(start, sizeof(start),
// 			"HTTP/1.1 %d %s \r\n"
// 			"Content-Length: %zu\r\n"
// 			"Connection: %s\r\n",
// 			status, reason.c_str(), content_len, close ? "close" : "keep-alive");
	
// 	std::string	out(start, start + n);

// 	for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
// 	{
// 		const std::string&	k = it->first;
// 		if (k == "Connection" || k == "Content-Length")
// 			continue;
// 		out += k;
// 		out += ": ";
// 		out += it->second;
// 		out += "\r\n";
// 	}

// 	out += "\r\n";

// 	if (!head_only)
// 		out += body;

// 	return (out);
// }

// const char* HttpResponse::defaultReason(int code)
// {
// 	switch (code)
// 	{
// 		case 200: return ("OK");
// 		case 204: return ("No Content");
// 		case 301: return ("Moved Permanently");
// 		case 302: return ("Found");
// 		case 400: return ("Bad Request");
// 		case 404: return ("Not Found");
// 		case 405: return ("Method Not Allowed");
// 		case 413: return ("Payload Too Large");
// 		case 431: return ("Request Header Fields Too Large");
// 		case 500: return ("Internal Server Error");
// 		case 501: return ("Not Implemented");
// 		default: return ("");
// 	}
// }
