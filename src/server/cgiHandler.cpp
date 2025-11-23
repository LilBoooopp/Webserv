#include "cgiHandler.hpp"

#include "../utils/Chrono.hpp"
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

static std::string execute(const std::string &file, const std::string &args, size_t timeoutMs,
			   size_t maxOutput) {
	int fds[2];
	if (pipe(fds) == -1) {
		Logger::error("Pipe error");
		return "";
	}

	pid_t pid = fork();
	if (pid == -1) {
		close(fds[0]);
		close(fds[1]);
		Logger::error("Fork error");
		return "";
	}

	if (pid == 0) {
		if (dup2(fds[1], STDOUT_FILENO) == -1) {
			Logger::error("dup2 error");
			_exit(1);
		}
		close(fds[0]);
		close(fds[1]);

		char *argv[2];
		argv[0] = const_cast<char *>(file.c_str());
		argv[1] = NULL;

		std::string qs = std::string("QUERY_STRING=") + args;
		char *envp[2];
		envp[0] = const_cast<char *>(qs.c_str());
		envp[1] = NULL;

		execve(file.c_str(), argv, envp);
		_exit(1);
	}

	close(fds[1]);

	int flags = fcntl(fds[0], F_GETFL, 0);
	if (flags != -1)
		fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);

	std::string out;
	char buf[4096];
	size_t total = 0;
	unsigned long startMs = now_ms();
	int status = 0;

	for (;;) {
		ssize_t n = read(fds[0], buf, sizeof(buf));
		if (n > 0) {
			out.append(buf, n);
			total += n;
			if (total > maxOutput) {
				Logger::error(
				    "cgi stopping %s execution after reading %lu bytes (max %lu)",
				    file.c_str(), (unsigned long)total, (unsigned long)maxOutput);
				kill(pid, SIGKILL);
				break;
			}
		} else if (n == 0) {
			break;
		} else {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				break;
			}
		}

		pid_t r = waitpid(pid, &status, WNOHANG);
		if (r == pid) {
			if (n == 0)
				break;
		}

		if (timeoutMs > 0) {
			unsigned long nowMs = now_ms();
			if (nowMs - startMs >= timeoutMs) {
				unsigned long elapsed = nowMs - startMs;
				Logger::error("stopping %s execution - timed out after %lums",
					      file.c_str(), elapsed);
				kill(pid, SIGKILL);
				waitpid(pid, &status, 0);
				break;
			}
		}

		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			usleep(1000);
	}

	close(fds[0]);
	waitpid(pid, &status, WNOHANG);
	Logger::info("%s%s%s execution ended after %lums - raw output:\n\'%s\'", YELLOW, file.c_str(), TS,
		     (unsigned long)(now_ms() - startMs), out.c_str());

	return out;
}

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

std::string cgiHandler::runCgi(const std::string &pathToCgi, const std::string &queryString,
			       HttpResponse &res, const ServerConfig &cfg) {
	time_t timeout_ms = cfg.cgi_timeout_ms;
	size_t maxOutput = cfg.cgi_maxOutput;
	Logger::simple("%s CGI execution Data:\n  %sscript %s\n  args \'%s\'\n  timeout: %lums\n  "
		       "maxOutput %lu MB",
		       SERVER, GREY, pathToCgi.c_str(), queryString.c_str(), timeout_ms,
		       (unsigned long)(maxOutput / (1024UL * 1024UL)));

	std::string raw = execute(pathToCgi, queryString, timeout_ms, maxOutput);
	if (raw.empty()) {
		res.setStatus(502, "Bad Gateway");
		res.setContentType("text/plain");
		res.setBody("CGI produced no output");
		return "";
	}
	if (!parseCgiOutput(raw, res)) {
		res.setStatus(502, "Bad Gateway");
		res.setContentType("text/plain");
		res.setBody("Invalid CGI response");
		return "";
	}
	return raw;
}

std::string cgiHandler::extractArguments(std::string &pathToCgi) {
	std::string queryString = "";
	std::string::size_type qpos = pathToCgi.find('?');
	if (qpos == std::string::npos)
		return queryString;
	queryString = pathToCgi.substr(qpos + 1);
	pathToCgi = pathToCgi.substr(0, qpos);
	Logger::info("args %s\"%s\"%s extracted from %s", GREY, queryString.c_str(), TS,
		     pathToCgi.c_str());
	return queryString;
}
