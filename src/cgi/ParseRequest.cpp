#include "cgi.hpp"

std::string getInterpreter(const std::string &path, const ServerConf &conf) {
	size_t qpos = path.find('?');
	size_t searchEnd = (qpos == std::string::npos) ? path.size() : qpos;
	size_t dotPos = path.rfind('.', searchEnd);
	if (dotPos == std::string::npos)
		return "";
	std::string ext = path.substr(dotPos + 1, searchEnd - dotPos - 1);
	if (ext != "py" && ext != "php")
		return "";
	const LocationConf *loc = findLocation(conf, path);
	if (!loc)
		return "";
	if (ext == "py" && loc->has_py)
		return loc->py_path;
	if (ext == "php" && loc->has_php)
		return loc->php_path;
	return "";
}

bool is_cgi(const std::string &req_target, const ServerConf &cfg) {
	return (!std::strncmp(req_target.c_str(), "/cgi/", 4));
	std::string interpreter = getInterpreter(req_target, cfg);
	if (!interpreter.empty())
		return true;
}

static std::string extractArguments(std::string &pathToCgi) {
	std::string queryString = "";
	std::string::size_type qpos = pathToCgi.find('?');
	if (qpos == std::string::npos)
		return queryString;
	queryString = pathToCgi.substr(qpos + 1);
	pathToCgi = pathToCgi.substr(0, qpos);
	// Logger::info("args %s\"%s\"%s extracted from %s", GREY, queryString.c_str(), TS,
	// 	     pathToCgi.c_str());
	return queryString;
}

void parseCgiRequest(const std::string &target, std::string &dir, std::string &file,
		     std::string &queryString, const ServerConf &conf) {

	std::string joined = safe_join_under_root(conf.root, target);
	Logger::cgi("%s rooted %s%s%s -> %s%s", SERV_CLR, GREY, target.c_str(), TS, GREY,
		    joined.c_str());
	Logger::cgi("Parsing cgi request: \'%s\'\n", joined.c_str());
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
