#include "StaticHandler.hpp"
#include "../config/Config.hpp"
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Mime.hpp"
#include "../utils/Path.hpp"

#include "../utils/Logger.hpp"
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static bool is_dir(const struct stat &st) { return (S_ISDIR(st.st_mode)); }
static bool is_reg(const struct stat &st) { return (S_ISREG(st.st_mode)); }

// Threshold above which we stream instead of loading into memory
static const off_t STREAM_THRESHOLD = 256 * 1024; // 256 KB

/**
 * @brief Convert a file size (off_t) to std::string
 */
static std::string size_to_str(off_t n) {
  std::ostringstream oss;
  oss << n;
  return (oss.str());
}

/**
 * @brief Read entire file into 'out'.
 * @return true on sucess, false on any open/read failure.
 */
static bool read_file_small(const std::string &path, std::string &out) {
  Logger::info("GET file path: %s", path.c_str());

  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0)
    return (false);

  std::vector<char> buf(64 * 1024);
  out.clear();

  bool got_any = false;

  for (;;) {
    ssize_t r = ::read(fd, &buf[0], buf.size());
    if (r > 0) {
      got_any = true;
      out.append(&buf[0], r);
    } else
      break;
  }
  ::close(fd);
  return (got_any);
}

void StaticHandler::handle(Connection &c, const HttpRequest &req,
                           HttpResponse &res) {
  // We assume: method validated (ONLY GET for now)
  const bool is_head = (req.method == "HEAD");
  (void)c;

  std::string request_path = req.target;
  std::string root_dir = server_conf_.root;
  const std::vector<std::string> *index_files =
      &server_conf_.files; // WARNING: index list??

  if (location_conf_) {
    size_t loc_path_len = location_conf_->path.size();
    if (loc_path_len > 0) {
      request_path.erase(0, loc_path_len);
    }
    if (!request_path.empty() && request_path[0] == '/')
      request_path.erase(0, 1);

    if (location_conf_->has_root)
      root_dir = location_conf_->root;

    if (location_conf_->has_index)
      index_files = &location_conf_->index_files;
  }

  // Map the request target to a safe filesystem path under cfg.root
  std::string path = safe_join_under_root(root_dir, request_path);

  // Check mime type
  res.setContentType(mime_from_path(path));

  Logger::info("%s rooted %s%s%s -> %s%s", SERV_CLR, GREY, req.target.c_str(),
               TS, GREY, path.c_str());

  struct stat st;
  if (::stat(path.c_str(), &st) == 0) {
    // Is a Directory
    if (is_dir(st)) {
      // try index file: root/target[/] + cfg.index
      if (path.size() == 0 || path[path.size() - 1] != '/')
        path += '/';

      if (!index_files->empty()) {
        std::string idx = path + index_files->at(0);

        if (::stat(idx.c_str(), &st) == 0 && is_reg(st)) {
          if (is_head) {
            res.setStatus(200, "OK");
            res.setHeader("Content_Length", size_to_str(st.st_size));
            return;
          }

          if (st.st_size <= STREAM_THRESHOLD) {
            std::string body;
            if (read_file_small(idx, body)) {
              res.setStatus(200, "OK");
              res.setBody(body);
              return;
            }
          } else {
            res.setStatus(200, "OK");
            res.setHeader("Content_Length", size_to_str(st.st_size));
            res.setHeader("X-Stream-File", idx);
            return;
          }
        }
      }

      res.setStatusFromCode(404);
      res.ensureDefaultBodyIfEmpty();
      return;

    }
    // Is a Regular File
    else if (is_reg(st)) {
      // For HEAD, we don't read the file, we just set headers
      if (is_head) {
        res.setStatus(200, "OK");
        // Use stat's size for Content_Length
        res.setHeader("Content-Length", size_to_str(st.st_size));
        return;
      }

      // Decide between small read vs streaming
      if (st.st_size <= STREAM_THRESHOLD) {
        std::string body;
        if (read_file_small(path, body)) {
          res.setStatus(200, "OK");
          res.setBody(body);
          // For GET, HttpResponse::serialize() can establish Content-Length
          // from body.size()
          return;
        }
      } else {
        // Large file: mark for streaming
        res.setStatus(200, "OK");
        res.setHeader("Content-Length", size_to_str(st.st_size));
        // Internal hint for Server: path to stream
        res.setHeader("X-Stream-File", path);
        return;
      }
    }
  }
  res.setStatusFromCode(404);
  res.ensureDefaultBodyIfEmpty();
}
