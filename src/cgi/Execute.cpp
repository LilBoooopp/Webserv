#include "../utils/Chrono.hpp"
#include "../utils/Path.hpp"
#include "cgi.hpp"
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

extern char **environ;

static bool execute(CgiExecutionData &data) {
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
		return false;
	}

	if (pid == 0) {
		if (dup2(fds[1], STDOUT_FILENO) == -1)
			_exit(1);
		close(fds[0]);
		close(fds[1]);

		chdir(data.path.c_str());
		setenv("QUERY_STRING", data.queryString.c_str(), 1);
		if (!data.interpreter.empty()) {
			char *argv[3];
			argv[0] = const_cast<char *>(data.interpreter.c_str());
			argv[1] = const_cast<char *>(data.file.c_str());
			argv[2] = NULL;
			execve(data.interpreter.c_str(), argv, environ);
		} else {
			char *argv[2];
			argv[0] = const_cast<char *>(data.file.c_str());
			argv[1] = NULL;
			execve(data.file.c_str(), argv, environ);
		}
		_exit(1);
	}
	close(fds[1]);
	fcntl(fds[0], F_SETFL, O_NONBLOCK);
	data.readFd = fds[0];
	data.pid = pid;
	data.bytesRead = 0;
	data.start = now_ms();
	Logger::timer("cgi execution successfully started, added data to cgiResponses");

	return true;
}

void cgiHandler::runCgi(const HttpRequest &req, HttpResponse &res, Connection &c, int fd) {
	if (!cfg_) {
		Logger::error("cgiHandler used without config!");
		return;
	}
	std::string parseRequest = safe_join_under_root(cfg_->root, req.target);
	std::string path, file, interpreter, queryString;
	parseCgiRequest(parseRequest, path, file, interpreter, queryString);
	struct stat st;
	std::string full = path + file;
	if (::stat(full.c_str(), &st) != 0) {
		res.setStatus(404, "Not Found");
		res.setBody("not found");
		res.setContentType("text/plain");
		Logger::debug("cgi file not found: \'%s\' target: \'%s', returning 404",
			      full.c_str(), req.target.c_str());
		return;
	}
	CgiExecutionData data;
	data.interpreter = interpreter;
	data.path = path;
	data.file = file;
	data.queryString = queryString;
	data.conn = &c;
	data.fd = fd;
	bool execSuccess = execute(data);
	Logger::simple("%s CGI execution Data - %s%s%s:\n"
		       "  %-12s %s\n"
		       "  %-12s %s\n"
		       "  %-12s %s\n"
		       "  %-12s '%s'\n"
		       "  %-12s %d\n",
		       SERVER, execSuccess ? GREEN : RED, execSuccess ? "Success" : "Failed", GREY,
		       "interpreter:", data.interpreter.c_str(), "path:", data.path.c_str(),
		       "file:", data.file.c_str(), "queryString:", data.queryString.c_str(), "pid",
		       data.pid);
	if (execSuccess)
		cgiResponses_.push_back(data);
}

void cgiHandler::setConfig(const ServerConfig &cfg) {
	cfg_ = &cfg;
	Logger::simple("%sCgiHandler%s\n  %-10s%lums\n  %-10s%lu MB\n", rgba(168, 145, 185, 1),
		       GREY, "timeout", (unsigned long)cfg_->cgi_timeout_ms, "maxOutput",
		       (unsigned long)(cfg_->cgi_maxOutput / (1024UL * 1024UL)));
}