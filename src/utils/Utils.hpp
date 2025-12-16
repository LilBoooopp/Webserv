#include <ctime>
#include <dirent.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>

inline bool path_exists(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
}

inline bool is_directory(const std::string &path) {
  struct stat buffer;
  if (stat(path.c_str(), &buffer) != 0)
    return (false);
  return (S_ISDIR(buffer.st_mode));
}

inline std::string generate_autoindex(const std::string &path,
                                      const std::string &request_target) {
  DIR *dir = opendir(path.c_str());
  if (!dir)
    return ("");

  std::stringstream ss;
  ss << "<html><head><title>Index of " << request_target
     << "</title></head><body>";
  ss << "<h1>Index of " << request_target << "</h1><hr><pre>";

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == ".")
      continue;

    std::string display_name = name;
    if (entry->d_type == DT_DIR)
      display_name += "/";

    ss << "<a href=\"" << display_name << "\">" << display_name << "</a><br>";
  }
  ss << "</pre><hr></body></html>";
  closedir(dir);
  return (ss.str());
}
