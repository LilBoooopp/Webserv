#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include <ctime>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct Session {
	std::string username;
	std::time_t created_at;
	std::time_t last_access;
	bool isAuthenticated;
	Session() : username(""), created_at(0), last_access(0), isAuthenticated(false) {}
	Session(const std::string &user)
	    : username(user), created_at(std::time(NULL)), last_access(created_at),
	      isAuthenticated(true) {};
};

class SessionManager {
    private:
	std::unordered_map<std::string, Session> sessions_;
	std::unordered_map<std::string, std::string> users;

    public:
	SessionManager() {};
	~SessionManager() {};
	const std::string createSession(const std::string &body);
	const std::string createSessionForUser(const std::string &username);
	Session *getSession(const std::string &sessionId, Session &out);
	bool hasSession(const std::string &header);
	void destroySession(const std::string &sessionId);
	void cleanupExpiredSessions(int maxAgeSeconds);
	void listSessions();
	bool authorize(const std::string &username, const std::string &password);
};

std::unordered_map<std::string, std::string> parseUrlEncoded(const std::string &body);
