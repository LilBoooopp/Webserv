#include "../utils/Chrono.hpp"
#include "cgi.hpp"
#include <cstdlib>

static void trim_spaces(std::string &s) {
	std::string::size_type start = 0;
	while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
		++start;
	std::string::size_type end = s.size();
	while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
		--end;
	s.assign(s, start, end - start);
}

static bool parseCgiOutput(const std::string &raw, HttpResponse &res) {
	std::string::size_type pos = raw.find("\r\n\r\n");
	std::string::size_type sep_len = 4;
	if (pos == std::string::npos) {
		pos = raw.find("\n\n");
		sep_len = 2;
	}
	if (pos == std::string::npos)
		return false;

	std::string headerBlock(raw, 0, pos);
	std::string body(raw, pos + sep_len);

	std::string::size_type start = 0;
	bool hasStatus = false;
	bool hasContentType = false;

	while (start < headerBlock.size()) {
		std::string::size_type end = headerBlock.find('\n', start);
		if (end == std::string::npos)
			end = headerBlock.size();
		std::string line(headerBlock, start, end - start);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		if (line.empty())
			break;

		std::string::size_type colon = line.find(':');
		if (colon != std::string::npos) {
			std::string name(line, 0, colon);
			std::string value(line, colon + 1);
			trim_spaces(name);
			trim_spaces(value);

			for (std::size_t i = 0; i < name.size(); ++i)
				name[i] = static_cast<char>(
				    std::tolower(static_cast<unsigned char>(name[i])));

			if (name == "status") {
				int code = 0;
				std::string reason;
				std::string::size_type sp = value.find(' ');
				if (sp == std::string::npos) {
					code = std::atoi(value.c_str());
					reason = "";
				} else {
					code = std::atoi(value.substr(0, sp).c_str());
					reason = value.substr(sp + 1);
				}
				if (code <= 0)
					code = 200;
				if (reason.empty())
					reason = "OK";
				res.setStatus(code, reason);
				hasStatus = true;
			} else if (name == "content-type") {
				res.setContentType(value);
				hasContentType = true;
			} else {
				res.setHeader(name, value);
			}
		}

		if (end == headerBlock.size())
			break;
		start = end + 1;
	}

	if (!hasStatus)
		res.setStatus(200, "OK");
	if (!hasContentType)
		res.setContentType("text/html");

	res.setBody(body);
	return true;
}

bool cgiHandler::handleResponses() {
	if (cgiResponses_.empty())
		return false;

	bool anyProgress = false;
	size_t i = 0;
	while (i < cgiResponses_.size()) {
		CgiExecutionData &data = cgiResponses_[i];
		char buf[4096];
		bool finished = false;

		std::string err;
		ssize_t n = read(data.readFd, buf, sizeof(buf));
		if (n > 0) {
			Logger::info("%d bytes read from fd %d", n, data.readFd);
			data.out.append(buf, n);
			data.bytesRead += n;
			anyProgress = true;
			if (data.bytesRead > cfg_->cgi_maxOutput) {
				err = "CGI output exceeded server limit";
				Logger::error("cgi stopping %s execution after %lu bytes (max %lu)",
					      data.file.c_str(), (unsigned long)data.bytesRead,
					      (unsigned long)cfg_->cgi_maxOutput);
				kill(data.pid, SIGKILL);
				finished = true;
			}
		} else if (n == 0)
			finished = true;
		else if (errno != EAGAIN && errno != EWOULDBLOCK)
			finished = true;
		unsigned long nowMs = now_ms();
		if (!finished && cfg_->cgi_timeout_ms > 0 &&
		    nowMs - data.conn->start >= cfg_->cgi_timeout_ms) {
			unsigned long elapsed = nowMs - data.conn->start;
			err = "CGI timed out";
			Logger::error("stopping \'%s%s%s\' execution - timed out after %lums",
				      YELLOW, data.file.c_str(), TS, elapsed);
			kill(data.pid, SIGKILL);
			finished = true;
		}

		if (finished) {
			int status = 0;
			waitpid(data.pid, &status, WNOHANG);
			close(data.readFd);
			Connection *conn = data.conn;
			if (conn) {
				HttpResponse res(200);
				if (err.empty()) {
					if (data.out.empty())
						err = "CGI produced no output";
					else if (!parseCgiOutput(data.out, res))
						err = "Invalid CGI response";
				}
				if (!err.empty()) {
					res.setStatus(502, "Bad Gateway");
					res.setContentType("text/plain");
					res.setBody(err);
				}
				bool head_only = (conn->req.method == "HEAD");
				std::string head = res.serialize(true);
				std::string preview = data.out.substr(0, 50);
				Logger::info("%s%s%s execution ended after %lums - raw output "
					     "(first 50 bytes):\n'%s'",
					     YELLOW, data.file.c_str(), TS,
					     (unsigned long)(nowMs - data.conn->start), preview.c_str());
				conn->out = res.serialize(head_only);
				conn->state = WRITING_RESPONSE;
			}
			cgiResponses_.erase(cgiResponses_.begin() + i);
		} else {
			++i;
		}
	}
	// if (anyProgress) {
	// 	Logger::info("cgiResponses: %zu cgi currently running", cgiResponses_.size());
	// }

	return anyProgress;
}
