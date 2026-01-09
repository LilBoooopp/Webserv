#pragma once
#include <string>
#include <sys/stat.h>

inline bool file_exists(const std::string &path) {
	struct stat st;
	return ::stat(path.c_str(), &st) == 0;
}

inline bool is_dir(const struct stat &st) { return S_ISDIR(st.st_mode); }
inline bool is_reg(const struct stat &st) { return S_ISREG(st.st_mode); }

inline bool is_dir(const std::string &path) {
	struct stat st;
	if (::stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}
inline bool is_reg(const std::string &path) {
	struct stat st;
	if (::stat(path.c_str(), &st) != 0)
		return false;
	return S_ISREG(st.st_mode);
}
