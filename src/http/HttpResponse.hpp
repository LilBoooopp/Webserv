#pragma once
#include <string>
#include <map>
#include <sstream>

class HttpResponse {
public:
	HttpResponse(int statusCode = 200, const std::string& reason = "OK");
	int			getStatus();
	void		setStatus(int code, const std::string& reason);
	void		setBody(const std::string& body);
	void		setContentType(const std::string& type);
	void		setHeader(const std::string& key, const std::string& value);
	std::string	serialize(bool headOnly = false) const;

private:
	int									statusCode_;
	std::string							reasonPhrase_;
	std::map<std::string,std::string>	headers_;
	std::string							body_;

	static std::string	toString(size_t n);
};
