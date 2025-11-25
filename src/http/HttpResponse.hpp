#pragma once
#include <string>
#include <map>

class HttpResponse {
public:
	HttpResponse(int statusCode = 200, const std::string& version = "HTTP/1.1");

	int			getStatus(void);
	void		setVersion(const std::string& version);
	void		setStatus(int code, const std::string& reason);
	void		setStatusFromCode(int code);
	void		setBody(const std::string& body);
	void		setBodyIfEmpty(const std::string& body);
	void		ensureDefaultBodyIfEmpty();
	void		setContentType(const std::string& type);

	void		setHeader(const std::string& key, const std::string& value);
	bool		hasHeader(const std::string& key) const;
	void		eraseHeader(const std::string& key);
	std::string	getHeader(const std::string& key) const;

	std::string	serialize(bool headOnly = false) const;

private:
	std::string							version_;
	int									statusCode_;
	std::string							reasonPhrase_;
	std::map<std::string,std::string>	headers_;
	std::string							body_;

	static std::string	toString(size_t n);
	static const char *reasonForStatus(int code);
};
