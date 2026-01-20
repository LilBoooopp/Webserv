#include "Cgi.hpp"
#include <cstring>


std::string getInterpreter(const std::string &path, const ServerConf &conf) {
    size_t qpos = path.find('?');
    size_t searchEnd = (qpos == std::string::npos) ? path.size() : qpos;
    std::string cleanPath = path.substr(0, searchEnd);
    size_t dotPos = cleanPath.rfind('.');

    if (dotPos == std::string::npos)
        return "";
    std::string ext = cleanPath.substr(dotPos); // ".py" or ".php"
    if (ext != ".py" && ext != ".php")
        return "";

    const LocationConf *loc = Router::matchLocation(conf, cleanPath);
    if (!loc)
        return "";

    std::map<std::string, std::string>::const_iterator it = loc->cgi.find(ext);
    if (it != loc->cgi.end())
        return it->second;

    return "";
}

// std::string getInterpreter(const std::string &path, const ServerConf &conf) {
// 	size_t qpos = path.find('?');
// 	size_t searchEnd = (qpos == std::string::npos) ? path.size() : qpos;
// 	size_t dotPos = path.rfind('.', searchEnd);
// 	if (dotPos == std::string::npos)
// 		return "";
// 	std::string ext = path.substr(dotPos + 1, searchEnd - dotPos - 1);
// 	if (ext != "py" && ext != "php")
// 		return "";
// 	const LocationConf *loc = Router::matchLocation(conf, path);
// 	if (!loc)
// 		return "";
// 	std::map<std::string, std::string>::const_iterator it = loc->cgi.find(ext);
// 	if (it != loc->cgi.end())
// 		return it->second;
//     std::map<std::string, std::string>::const_iterator it = loc->cgi.find(ext);
//     if (it != loc->cgi.end())
//         return it->second;
// 	// if (ext == "py" && loc->has_py)
// 	// 	return loc->py_path;
// 	// if (ext == "php" && loc->has_php)
// 	// 	return loc->php_path;
// 	return "";
// }

static std::string extractArguments(std::string &pathToCgi) {
	std::string queryString = "";
	std::string::size_type qpos = pathToCgi.find('?');
	if (qpos == std::string::npos)
		return queryString;
	queryString = pathToCgi.substr(qpos + 1);
	pathToCgi = pathToCgi.substr(0, qpos);
	return queryString;
}

bool is_cgi(const std::string &req_target, const ServerConf &cfg) {
	(void)cfg;
	return (std::strncmp(req_target.c_str(), "/cgi-bin/", 5) == 0);
	// if (std::strncmp(req_target.c_str(), "/cgi-bin/", 5) != 0)
	// 	return false;
	// return !getInterpreter(req_target, cfg).empty();
}

bool CgiData::tryInit(Connection &c, int fd) {
	method = c.req.method;
	requestUri = c.req.target;
	contentType = c.req.getHeader("Content-Length");
	contentLength = c.req.getHeader("Content-Length");
	if (contentLength.empty())
		contentLength = to_string(c.body.size());
	this->fd = fd;
	conn = &c;
	parseCgiRequest(requestUri, this->path, this->file, this->queryString, c.cfg);
	interp = getInterpreter(requestUri, c.cfg);
	if (interp.empty()) {
		Logger::cgi("No Interpreter found for cgi request's target '%s'",
			    requestUri.c_str());
		return false;
	}
	std::string full = this->path + this->file;
	if (!file_exists(full)) {
		Logger::cgi("file not found: \'%s\' target: \'%s', returning 404", full.c_str(),
			    requestUri.c_str());
		return false;
	}
	const LocationConf *loc = Router::matchLocation(c.cfg, requestUri);
	if (loc) {
		timeout_ms = loc->cgi_timeout_ms;
		maxOutput = loc->cgi_maxOutput;
	}
	return true;
}

void parseCgiRequest(const std::string &target, std::string &dir, std::string &file,
		     std::string &queryString, const ServerConf &conf) {

	std::string joined = safe_join_under_root(conf.root, target);
	Logger::cgi("%srouted %s%s%s -> %s%s", GREY, URLCLR, target.c_str(), GREY, URLCLR,
		    joined.c_str());
	Logger::cgi("%sParsing cgi request: \'%s%s%s\'", GREY, URLCLR, joined.c_str(), GREY);
	queryString = extractArguments(joined);

	size_t pos = joined.find_last_of('/');
	if (pos == std::string::npos) {
		dir = "";
		file = joined;
		return;
	}
	dir = joined.substr(0, pos + 1);
	file = joined.substr(pos + 1);
}

void CgiHandler::setConfig(const std::vector<ServerConf> &cfg) {
	cfg_ = &cfg;
	Logger::cgi("%sCgiHandler%s\n  %-10s%lums\n  %-10s%lu MB\n", rgba(168, 145, 185, 1), GREY,
		    "timeout", (unsigned long)(*cfg_)[0].locations[0].cgi_timeout_ms, "maxOutput",
		    (unsigned long)((*cfg_)[0].locations[0].cgi_maxOutput / (1024UL * 1024UL)));
}

void CgiHandler::killAsyncProcesses() {
	for (size_t i = 0; i < asyncPids_.size(); ++i) {
		if (kill(asyncPids_[i], 0) == 0) { // Check if process exists
			Logger::cgi("Killing async CGI process: %d", asyncPids_[i]);
			kill(asyncPids_[i], SIGTERM);
		}
	}
	asyncPids_.clear();
}

void CgiData::log(bool execSuccess) {
	Logger::cgi("%s CGI execution Data - %s%s%s:\n"
		    "  %-12s %s\n"
		    "  %-12s %s\n"
		    "  %-12s %s\n"
		    "  %-12s '%s'\n"
		    "  %-12s %d\n",
		    SERV_CLR, execSuccess ? GREEN : RED, execSuccess ? "Success" : "Failed", GREY,
		    "interpreter:", interp.c_str(), "path:", path.c_str(), "file:", file.c_str(),
		    "queryString:", queryString.c_str(), "pid", pid);
}