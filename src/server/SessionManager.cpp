#include "SessionManager.hpp"
#include <functional>

std::string naiveHash(const std::string &s) {
	std::hash<std::string> hasher;
	std::ostringstream oss;
	oss << hasher(s);
	return oss.str();
}

static const std::string getSecureSessionId() {
	srand((unsigned int)time(NULL));
	std::string res;
	const int len = 20;
	for (int i = 0; i < len; i++) {
		char c = (rand() % (90 - 65)) + 65;
		if ((rand() % 2) == 0)
			c += 32;
		res += c;
	}
	return res;
}

std::unordered_map<std::string, std::string> parseUrlEncoded(const std::string &body) {
	std::unordered_map<std::string, std::string> out;

	std::stringstream ss(body);
	std::string pair;

	while (std::getline(ss, pair, '&')) {
		size_t eq = pair.find('=');
		if (eq == std::string::npos)
			continue;

		std::string key = pair.substr(0, eq);
		std::string val = pair.substr(eq + 1);

		out[key] = val;
	}

	return out;
}

const std::string SessionManager::createSession(const std::string &body) {

	std::unordered_map<std::string, std::string> form = parseUrlEncoded(body);
	std::string username = form["user"];
	std::string password = form["pass"];
	if (username.empty() || password.empty()) {
		Logger::auth("can't create session: %s not in request body",
			     username.empty() ? "username" : "password");
		return "";
	}
	std::string sessionId = getSecureSessionId();
	Session s(username);
	users[username] = naiveHash(password);
	sessions_[sessionId] = s;
	Logger::auth("Session created [user=%s, id=%s]", username.c_str(), sessionId.c_str());
	return sessionId;
}

const std::string SessionManager::createSessionForUser(const std::string &username) {
	std::string sessionId = getSecureSessionId();
	Session s(username);
	sessions_[sessionId] = s;
	Logger::auth("Session created [user=%s, id=%s]", username.c_str(), sessionId.c_str());
	return sessionId;
}

Session *SessionManager::getSession(const std::string &sessionId, Session &out) {
	std::unordered_map<std::string, Session>::iterator it = sessions_.find(sessionId);
	if (it == sessions_.end())
		return nullptr;

	it->second.last_access = std::time(NULL);
	out = it->second;
	return &it->second;
}

bool SessionManager::hasSession(const std::string &cookieHeader) {
	std::string sessionId;
	std::stringstream ss(cookieHeader);
	std::string part;
	while (std::getline(ss, part, ';')) {
		size_t start = part.find_first_not_of(" \t");
		if (start == std::string::npos)
			continue;
		part = part.substr(start);
		size_t eq = part.find('=');
		if (eq == std::string::npos)
			continue;
		std::string key = part.substr(0, eq);
		std::string val = part.substr(eq + 1);
		if (key == "sessionId") {
			sessionId = val;
			break;
		}
	}
	if (sessionId.empty())
		return false;
	std::unordered_map<std::string, Session>::iterator sessionKey = sessions_.find(sessionId);
	return sessionKey != sessions_.end();
}

void SessionManager::destroySession(const std::string &sessionId) { sessions_.erase(sessionId); }

void SessionManager::cleanupExpiredSessions(int maxAgeSeconds) {
	std::time_t now = std::time(NULL);

	for (std::unordered_map<std::string, Session>::iterator it = sessions_.begin();
	     it != sessions_.end();) {
		if (now - it->second.last_access > maxAgeSeconds)
			it = sessions_.erase(it);
		else
			++it;
	}
}

void SessionManager::listSessions() {
	const int n = sessions_.size();
	Logger::timer("%d %sSession%c", n, PURPLE, n > 1 ? 's' : ' ');
	for (std::unordered_map<std::string, Session>::iterator it = sessions_.begin();
	     it != sessions_.end(); it++) {
		Logger::simple(" SessionId %s", it->first.c_str());
		Logger::simple("%s  Username: %s\n  authentified: %d\n  createdAt: "
			       "%zu\n  lastAccess: %zu",
			       GREY, it->second.username.c_str(), it->second.isAuthenticated,
			       it->second.created_at, it->second.last_access);
	}
}

bool SessionManager::authorize(const std::string &username, const std::string &password) {
	std::unordered_map<std::string, std::string>::iterator it = users.find(username);
	if (it == users.end())
		return false;
	const std::string hashed = it->second;
	return hashed == naiveHash(password);
}