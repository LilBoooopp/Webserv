// #pragma once
// #include "HttpResponse.hpp"
// #include <string>
// #include <cstdio>

// struct	ResponseWriter {
// 	static std::string	toWire(const HttpResponse& r) {
// 		char	head[256];
// 		int		n = std::snprintf(head, sizeof(head),
// 				"HTTP/1.1 %d %s\r\n"
// 				"Content-Length: %zu\r\n"
// 				"Content-Type: %s\r\n"
// 				"Connection: %s \r\n"
// 				"\r\n",
// 				r.status, r.reason.c_str(), r.body.size(),
// 				r.contentType.empty() ? "text/plain" : r.contentType.c_str(),
// 				r.close ? "close" : "keep alive");
// 		return (std::string(head, head + n) + r.body);
// 	}
// };
