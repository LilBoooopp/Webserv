#include "Cgi.hpp"
#include "CgiData.hpp"
#include <cstdlib>

static void trim_spaces(std::string &s) {
  std::string::size_type start = 0;
  while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
    ++start;
  std::string::size_type end = s.size();
  while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
    --end;
  s.assign(s, start, end - start);
}

static bool scan_headers(int fd, std::string &header, off_t &body_offset,
                         size_t limit) {
  ssize_t cur_read;
  off_t total_read = 0;

  std::string p_sep = "\r\n\r\n";
  std::string s_sep = "\n\n";

  size_t p_pos = std::string::npos;
  size_t s_pos = std::string::npos;

  char buf[4000];

  while (true) {
    cur_read = read(fd, buf, sizeof(buf));
    if (cur_read < 0)
      return (false);
    if (cur_read == 0)
      break;
    total_read += cur_read;
    header.append(buf, cur_read);

    p_pos = header.find(p_sep);
    if (p_pos != std::string::npos) {
      body_offset = total_read - (header.size() - (p_pos + 4));
      header.erase(p_pos);
      return (true);
    }
    if (s_pos == std::string::npos)
      s_pos = header.find(s_sep);

    if (limit != 0 && header.size() > limit)
      break;
  }
  if (s_pos != std::string::npos) {
    body_offset = total_read - (header.size() - (s_pos + 2));
    header.erase(s_pos);
    return (true);
  }
  return (false);
}

static bool get_body_len(size_t total_size, off_t body_offset,
                         off_t &body_len) {
  if (body_offset < 0)
    return false;

  if (body_offset > (off_t)total_size)
    return false;

  body_len = (off_t)total_size - body_offset;
  return true;
}

static bool parseCgiOutput(const std::string &headerBlock, HttpResponse &res) {
  std::string::size_type start = 0;
  bool hasStatus = false;
  bool hasContentType = false;

  while (start < headerBlock.size()) {
    std::string::size_type end = headerBlock.find('\n', start);
    if (end == std::string::npos)
      end = headerBlock.size();
    std::string line(headerBlock, start, end - start);
    if (!line.empty() && line[line.size() - 1] == '\r')
      line.erase(line.size() - 1);

    if (line.empty())
      break;

    std::string::size_type colon = line.find(':');
    if (colon != std::string::npos) {
      std::string name(line, 0, colon);
      std::string value(line, colon + 1);
      trim_spaces(name);
      trim_spaces(value);

      for (std::size_t i = 0; i < name.size(); ++i)
        name[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(name[i])));

      if (name == "status") {
        int code = 0;
        std::string reason;
        std::string::size_type sp = value.find(' ');
        if (sp == std::string::npos) {
          code = std::atoi(value.c_str());
          reason = "";
        } else {
          code = std::atoi(value.substr(0, sp).c_str());
          reason = value.substr(sp + 1);
        }
        if (code <= 0)
          code = 200;
        if (reason.empty())
          reason = "OK";
        res.setStatus(code, reason);
        hasStatus = true;
      } else if (name == "content-type") {
        res.setContentType(value);
        hasContentType = true;
      } else {
        res.setHeader(name, value);
      }
    }

    if (end == headerBlock.size())
      break;
    start = end + 1;
  }

  if (!hasStatus)
    res.setStatus(200, "OK");
  if (!hasContentType)
    res.setContentType("text/html");
  return true;
}

void CgiHandler::handleMessage(int fd) {
  std::map<int, CgiData>::iterator it = cgiResponses_.find(fd);

  if (it == cgiResponses_.end()) {
    if (reactor_)
      reactor_->del(fd);
    close(fd);
    return;
  }

  CgiData &data = it->second;
  char buf[4096];
  bool finished = false;
  std::string err;
  ssize_t n = 0;
  off_t header_off;
  off_t body_len;
  std::string header_text;

  if (data.tmp_fd < 0) {
    err = "Error opening tmp file";
    Logger::cgi(
        "%sstopping %s%s%s execution after %s%lu bytes%s (max %lu bytes)", GREY,
        YELLOW, data.file.c_str(), GREY, RED, (unsigned long)data.bytesRead,
        GREY, (unsigned long)data.maxOutput);
    kill(data.pid, SIGKILL);
    finished = true;
  } else
    n = read(fd, buf, sizeof(buf));

  if (n > 0) {
    Logger::cgi("%s%d bytes%s read from %s%s%s's output [fd %d]", VALUECLR, n,
                GREY, URLCLR, data.file.c_str(), GREY, data.readFd);
    // data.out.append(buf, n);
    ssize_t written = write(data.tmp_fd, buf, n);
    data.bytesRead += n;

    if (written < 0) {
      close(data.tmp_fd);
      data.tmp_fd = -1;
      err = "Error writing to tmp file";
      Logger::cgi(
          "%sstopping %s%s%s execution after %s%lu bytes%s (max %lu bytes)",
          GREY, YELLOW, data.file.c_str(), GREY, RED,
          (unsigned long)data.bytesRead, GREY, (unsigned long)data.maxOutput);
      kill(data.pid, SIGKILL);
      finished = true;
    } else if (data.bytesRead > data.maxOutput) {
      err = "CGI output exceeded server limit";
      Logger::cgi(
          "%sstopping %s%s%s execution after %s%lu bytes%s (max %lu bytes)",
          GREY, YELLOW, data.file.c_str(), GREY, RED,
          (unsigned long)data.bytesRead, GREY, (unsigned long)data.maxOutput);
      kill(data.pid, SIGKILL);
      finished = true;
    }
  } else if (n == 0) {
    finished = true;
  } else {
    err = "Read error on CGI pipe";
    finished = true;
  }

  if (finished) {
    if (reactor_)
      reactor_->del(fd);
    close(fd);

    int status = 0;
    waitpid(data.pid, &status, WNOHANG);

    if (data.conn) {
      HttpResponse &res = data.conn->res;

      res.setStatusFromCode(200);

      if (err.empty()) {
        close(data.tmp_fd);
        data.tmp_fd = -1;
        int tmp_fd = open(data.tmp_filename.c_str(), O_RDONLY);
        if (tmp_fd < 0) {
          err = "Error opening tmp file";
          Logger::cgi(
              "%sstopping %s%s%s execution after %s%lu bytes%s (max %lu bytes)",
              GREY, YELLOW, data.file.c_str(), GREY, RED,
              (unsigned long)data.bytesRead, GREY,
              (unsigned long)data.maxOutput);
          finished = true;
        } else {
          if (!scan_headers(tmp_fd, header_text, header_off, 0)) {
            err = "Invalid CGI output";
            Logger::cgi("%sstopping %s%s%s execution after %s%lu bytes%s (max "
                        "%lu bytes)",
                        GREY, YELLOW, data.file.c_str(), GREY, RED,
                        (unsigned long)data.bytesRead, GREY,
                        (unsigned long)data.maxOutput);
            finished = true;
          }
          if (!get_body_len(data.bytesRead, header_off, body_len)) {
            err = "Invalid CGI output";
            Logger::cgi("%sstopping %s%s%s execution after %s%lu bytes%s (max "
                        "%lu bytes)",
                        GREY, YELLOW, data.file.c_str(), GREY, RED,
                        (unsigned long)data.bytesRead, GREY,
                        (unsigned long)data.maxOutput);
            finished = true;
          }
          close(tmp_fd);
        }
        if (err.empty() && !parseCgiOutput(header_text, res)) {
          err = "Invalid CGI response";
          Logger::error("CGI %s output parsing failed.", data.file.c_str());
        } else if (err.empty()) {
          std::stringstream ss;
          ss << body_len;
          res.setHeader("Content-Length", ss.str());
          std::string connection_header =
              data.conn->req.getHeader("connection");
          if (connection_header == "Close" || connection_header == "close") {
            res.setHeader("Connection", "close");
          } else {
            res.setHeader("Connection", "keep-alive");
          }
          data.conn->out = res.serialize(true);
          if (data.conn->req.method == "HEAD") {
            data.conn->state = WRITING_RESPONSE;
            data.conn->cgi_out_path = data.tmp_filename;
            reactor_->mod(data.fd, EPOLLIN | EPOLLOUT);
          } else {
            data.conn->file_fd = open(data.tmp_filename.c_str(), O_RDONLY);
            if (data.conn->file_fd < 0) {
              err = "Error opening tmp file";
              Logger::cgi("%sstopping %s%s%s execution after %s%lu bytes%s "
                          "(max %lu bytes)",
                          GREY, YELLOW, data.file.c_str(), GREY, RED,
                          (unsigned long)data.bytesRead, GREY,
                          (unsigned long)data.maxOutput);
              finished = true;
            } else {
              data.conn->streaming_file = true;
              data.conn->file_skip = header_off;
              data.conn->file_remaining = body_len;
              data.conn->cgi_out_path = data.tmp_filename;
              data.conn->state = WRITING_RESPONSE;
              reactor_->mod(data.fd, EPOLLIN | EPOLLOUT);
            }
          }
        }
        // if (data.bytesRead == 0) {
        // 	err = "CGI produced no output";
        // 	Logger::error("CGI %s produced no output (0 bytes read)",
        // 		      data.file.c_str());
        // } else if (!parseCgiOutput(data.tmp_filename, res)) {
        // 	err = "Invalid CGI response";
        // 	Logger::error(
        // 	    "CGI %s output parsing failed. First 200 chars: %.*s",
        // 	    data.file.c_str(),
        // 	    (int)(data.out.size() < 200 ? data.out.size() : 200),
        // 	    data.out.c_str());
        // }
      }

      if (res.getStatus() >= 400) {
        Router::loadErrorPage(*(data.conn));
      }
      if (!err.empty()) {
        res.setStatusFromCode(502);
        res.setBodyIfEmpty(err);
        data.conn->out = res.serialize(data.conn->req.method == "HEAD");
        data.conn->state = WRITING_RESPONSE;
        reactor_->mod(data.fd, EPOLLIN | EPOLLOUT);
      }

      Logger::cgi("%s%s%s execution ended after %s%lums", YELLOW,
                  data.file.c_str(), GREY, LGREY,
                  (unsigned long)(now_ms() - data.conn->start));
    }
    cgiResponses_.erase(it); // Erase by iterator
  }
}

void CgiHandler::checkCgiTimeouts() {
  unsigned long now = now_ms();

  std::map<int, CgiData>::iterator it = cgiResponses_.begin();
  while (it != cgiResponses_.end()) {
    CgiData &data = it->second;

    if (data.conn == NULL) {
      Logger::cgi("Orphanned CGI process %d (connection closed), killing...",
                  data.pid);
      kill(data.pid, SIGKILL);
      waitpid(data.pid, NULL, WNOHANG);

      if (reactor_)
        reactor_->del(data.readFd);
      close(data.readFd);

      cgiResponses_.erase(it++);
      continue;
    }

    size_t limit = data.timeout_ms;

    if (limit > 0 && (now - data.start) > limit) {
      Logger::cgi("%s%s%s's execution timed-out after %s%lums%s (max "
                  "%s%lums%s), killing it and returning 504",
                  YELLOW, data.file.c_str(), GREY, RED,
                  (unsigned long)now - data.start, GREY, YELLOW,
                  (unsigned long)limit, GREY);
      kill(data.pid, SIGKILL);
      waitpid(data.pid, NULL, WNOHANG);

      if (reactor_)
        reactor_->del(data.readFd);
      close(data.readFd);

      data.conn->res.setStatus(504, "Gateway Timeout");
      Router::loadErrorPage(*(data.conn));
      data.conn->out = data.conn->res.serialize(false);
      data.conn->state = WRITING_RESPONSE;

      if (reactor_)
        reactor_->mod(data.fd, EPOLLIN | EPOLLOUT);
      cgiResponses_.erase(it++);
    } else {
      ++it;
    }
  }
}

void CgiHandler::detachConnection(Connection *conn) {
  if (!conn)
    return;
  for (std::map<int, CgiData>::iterator it = cgiResponses_.begin();
       it != cgiResponses_.end(); ++it) {
    if (it->second.conn == conn)
      it->second.conn = NULL;
  }
}
