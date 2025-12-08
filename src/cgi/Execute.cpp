#include "cgi.hpp"

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
		std::vector<std::string> envStrings;
		envStrings.push_back("REDIRECT_STATUS=200");
		envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
		envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
		envStrings.push_back("SERVER_SOFTWARE=webserv/1.0");
		envStrings.push_back("SERVER_NAME=127.0.0.1");
		envStrings.push_back("SERVER_PORT=8080");
		envStrings.push_back("REQUEST_URI=" + data.requestUri);
		envStrings.push_back("REQUEST_METHOD=" + data.method);
		envStrings.push_back("QUERY_STRING=" + data.queryString);
		envStrings.push_back("CONTENT_LENGTH=" + data.contentLength);
		envStrings.push_back("CONTENT_TYPE=" + data.contentType);
		envStrings.push_back("SCRIPT_NAME=" + data.file);
		envStrings.push_back("SCRIPT_FILENAME=" + data.file);

		for (std::map<std::string, std::string>::iterator it = data.headers.begin();
		     it != data.headers.end(); ++it) {
			std::string key = "HTTP_";
			for (size_t i = 0; i < it->first.size(); ++i) {
				char c = it->first[i];
				if (c == '-')
					key += '_';
				else
					key += std::toupper(static_cast<unsigned char>(c));
			}
			envStrings.push_back(key + "=" + it->second);
		}

		std::vector<char *> envp;
		for (size_t i = 0; i < envStrings.size(); ++i)
			envp.push_back(const_cast<char *>(envStrings[i].c_str()));
		envp.push_back(NULL);

		if (!data.interpreter.empty()) {
			char *argv[3];
			argv[0] = const_cast<char *>(data.interpreter.c_str());
			argv[1] = const_cast<char *>(data.file.c_str());
			argv[2] = NULL;
			execve(data.interpreter.c_str(), argv, &envp[0]);
		} else {
			char *argv[2];
			argv[0] = const_cast<char *>(data.file.c_str());
			argv[1] = NULL;
			execve(data.file.c_str(), argv, &envp[0]);
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

bool cgiHandler::runCgi(const HttpRequest &req, HttpResponse &res, Connection &c, int fd) {
	const ServerConf &cfg = (*cfg_)[c.serverIdx];

	std::string path, file, interpreter, queryString;
	interpreter = getInterpreter(req.target, cfg);
	if (interpreter.empty()) {
		Logger::cgi("No Interpreter found for cgi request's target \'%s\'",
			    req.target.c_str());
		res.setStatus(500, "No Interpreter");
		res.setBody("no interpreter");
		res.setContentType("text/plain");
		return false;
	}
	parseCgiRequest(req.target, path, file, queryString, cfg);
	struct stat st;
	std::string full = path + file;
	if (::stat(full.c_str(), &st) != 0) {
		Logger::cgi("file not found: \'%s\' target: \'%s', returning 404", full.c_str(),
			    req.target.c_str());
		res.setStatus(404, "Not Found");
		res.setBody("not found");
		res.setContentType("text/plain");

		return false;
	}
	CgiExecutionData data;
	data.method = req.method;
	data.requestUri = req.target;
	data.interpreter = interpreter;
	data.path = path;
	data.file = file;
	data.queryString = queryString;
	data.conn = &c;
	data.fd = fd;
	data.headers = c.req.headers;
	bool execSuccess = execute(data);
	Logger::cgi("%s CGI execution Data - %s%s%s:\n"
		    "  %-12s %s\n"
		    "  %-12s %s\n"
		    "  %-12s %s\n"
		    "  %-12s '%s'\n"
		    "  %-12s %d\n",
		    SERV_CLR, execSuccess ? GREEN : RED, execSuccess ? "Success" : "Failed", GREY,
		    "interpreter:", data.interpreter.c_str(), "path:", data.path.c_str(),
		    "file:", data.file.c_str(), "queryString:", data.queryString.c_str(), "pid",
		    data.pid);
	if (execSuccess)
		cgiResponses_.push_back(data);
	return true;
}

void cgiHandler::setConfig(const std::vector<ServerConf> &cfg) {
	cfg_ = &cfg;
	Logger::cgi("%sCgiHandler%s\n  %-10s%lums\n  %-10s%lu MB\n", rgba(168, 145, 185, 1), GREY,
		    "timeout", (unsigned long)(*cfg_)[0].locations[0].cgi_timeout_ms, "maxOutput",
		    (unsigned long)((*cfg_)[0].locations[0].cgi_maxOutput / (1024UL * 1024UL)));
}
