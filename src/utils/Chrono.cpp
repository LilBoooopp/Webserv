#include "Chrono.hpp"

std::string getTimestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	std::time_t now = tv.tv_sec;
	std::tm *tm = std::localtime(&now);

	int ms = tv.tv_usec / 1000;

	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << tm->tm_hour << ":" << std::setw(2)
	    << std::setfill('0') << tm->tm_min << ":" << std::setw(2) << std::setfill('0')
	    << tm->tm_sec << ":" << std::setw(3) << std::setfill('0') << ms;

	return oss.str();
}

std::string formatTime(time_t t) {
	char buf[32];
	std::tm *tm = std::localtime(&t);
	if (!tm)
		return "invalid";

	std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);
	return std::string(buf);
}