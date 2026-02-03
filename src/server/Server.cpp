#include "Server.hpp"
#include "../http/ChunkedDecoder.hpp"
#include "../http/HttpParser.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/Chrono.hpp"
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Path.hpp"
#include "Listener.hpp"
#include <algorithm>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

void Server::enableWrite(int fd) { reactor_.mod(fd, EPOLLIN | EPOLLOUT); }
void Server::disableWrite(int fd) { reactor_.mod(fd, EPOLLIN); }

void Server::cleanup() {
  Logger::server("Cleaning up server resources...");
  cgiHandler_.killAsyncProcesses();

  for (std::map<int, Connection>::iterator it = conns_.begin();
       it != conns_.end(); ++it) {
    reactor_.del(it->first);
    close(it->first);
  }
  conns_.clear();

  for (size_t i = 0; i < listener_.size(); ++i) {
    reactor_.del(listener_[i].fd());
    close(listener_[i].fd());
  }
  listener_.clear();

  Logger::server("Cleanup complete");
}

/**
 * @brief Starts the server by bind and listening on the host and ports
 *
 * @param ip_be
 * @param port_be
 * @param config
 * @return true if started successfully, false if not
 */
bool Server::start() {
  cfg_ = conf_.getServers();
  listener_.reserve(cfg_.size());
  uint32_t ip_be = cfg_[0].hosts.host;
  for (size_t i = 0; i < cfg_.size(); i++) {
    listener_.push_back(Listener());
    if (!listener_[i].bindAndListen(ip_be, cfg_[i].hosts.port)) {
      Logger::error("Bind and Listen error, IP %d PORT %d", ip_be,
                    cfg_[i].hosts.port);
      return (false);
    }
    if (!reactor_.add(listener_[i].fd(), EPOLLIN)) {
      Logger::error("reactor couldn't add listener at index %d", i);
      return (false);
    }
  }
  cgiHandler_.init(&reactor_);
  cgiHandler_.setConfig(cfg_);
  return (true);
}

/**
 * @brief Assigns incomming connections' fds to the previously bound listeners
 */
void Server::acceptReady(void) {
  for (size_t i = 0; i < listener_.size(); i++) {
    if (listener_[i].fd() < 0) {
      Logger::connection("unvalid fd %d", listener_[i].fd());
      return;
    }
    for (;;) {
      int cfd = ::accept(listener_[i].fd(), 0, 0);
      if (cfd < 0) {
        break;
      }
      set_nonblock(cfd);
      Connection c(cfg_[i]);
      c.start = now_ms();
      c.last_active = c.start;
      conns_.insert(std::make_pair(cfd, c));
      reactor_.add(cfd, EPOLLIN);
      Logger::connection("fd %d%s accepted", cfd, GREEN);
    }
  }
}

void Server::checkTimeouts() {
  unsigned long current_time = now_ms();

  std::map<int, Connection>::iterator it = conns_.begin();
  while (it != conns_.end()) {
    Connection &c = it->second;

    if (c.state == WAITING_CGI) {
      ++it;
      continue;
    }

    if ((current_time - c.last_active) > c.cfg.timeout_ms) {
      int fd = it->first;
      Logger::connection("FD %d timed out (idle for %lu ms)", fd,
                         (current_time - c.last_active));

      bool is_mid_request = (c.state == READING_BODY) ||
                            (c.state == READING_HEADERS && !c.in.empty());

      if (is_mid_request) {
        Logger::response("fd %d sending 408 Request Timeout", fd);

        HttpResponse timeout_res(408);
        timeout_res.setHeader("Connection", "close");
      }

      cgiHandler_.detachConnection(&c);
      reactor_.del(fd);
      ::close(fd);

      std::map<int, Connection>::iterator next = it;
      ++next;
      conns_.erase(it);
      it = next;
    } else {
      ++it;
    }
  }
}

/**
 * @brief Determines if a connection should be kept alive for HTTP/1.1
 * Keep-Alive
 *
 * @param c The connection to check
 * @return true if connection should be kept alive, false if it should be closed
 */
bool Server::shouldKeepConnectionAlive(Connection &c) {
  // Check if client requested Connection: keep-alive
  std::string connection_header = c.req.getHeader("Connection");
  if (connection_header != "keep-alive" && connection_header != "Keep-Alive") {
    return false;
  }

  // Check if server config allows keep-alive (default to true for HTTP/1.1)
  // For now, always allow keep-alive unless explicitly disabled
  // TODO: Add config option to disable keep-alive per server/location

  // Check if connection is in a state that allows reuse
  if (c.close_after || c.state == CLOSING) {
    return false;
  }

  // Check if response indicates connection should close
  if (c.res.getHeader("Connection") == "close") {
    return false;
  }

  return true;
}

/**
 * @brief Reads the bytes sent by the client as a whole or chunked if necessary
 *
 * @param fd of the socket associated with the current client being handled
 */
void Server::handleReadable(int fd) {
  Connection &c = conns_.at(fd);

  const size_t MAX_HANDLE_BYTES =
      16 * 1024; // max bytes to read from socket per iteration
  const size_t MAX_DECODE_BYTES =
      16 * 1024; // max bytes to consume from c.in for body parsin per iteration

  c.last_active = now_ms();

  size_t handled = 0;
  while (handled < MAX_HANDLE_BYTES) {
    ssize_t r = ::read(fd, &inbuf_[0], inbuf_.size());
    if (r > 0) {
      handled += static_cast<size_t>(r);
      c.in.append(&inbuf_[0], static_cast<size_t>(r));

      // Header cap defense
      // if (c.state == READING_HEADERS && c.in.size() > MAX_HANDLE_BYTES)
      //{
      //	HttpResponse	res(431);
      //	c.res.setContentType("text/plain");
      //	c.res.setBody("header too large");
      //	c.out = c.res.serialize(false);
      //	c.state = WRITING_c.resPONSE;
      //	enableWrite(fd);
      //	c.in.clear();
      //	break;
      //}

      // READING_HEADERS
      if (c.state == READING_HEADERS) {
        size_t eoh = c.in.find("\r\n\r\n");
        if (eoh != std::string::npos) {
          c.headers_done = true;

          size_t endpos = 0;

          if (!HttpParser::parse(c, endpos)) {
            if (c.req.version != "HTTP/1.1" && c.req.version != "HTTP/1.0" &&
                c.req.method != "" && c.req.target != "") {
              c.res.setStatusFromCode(505);
            } else {
              c.res.setStatusFromCode(400);
              c.res.setVersion(c.req.version);
            }
            c.state = WRITING_RESPONSE;
            c.in.clear();
          } else {
            bool bad = false;

            // TE/CL detection
            bool has_te_chunked = false;
            bool has_cl = false;
            size_t content_length = 0;

            // Check for Content-Length header
            std::string str = c.req.getHeader("content-length");
            if (!str.empty()) {
              has_cl = true;
              const std::string &s = str;
              size_t acc = 0;
              for (size_t k = 0; k < s.size(); ++k) {
                if (s[k] < '0' || s[k] > '9') {
                  acc = static_cast<size_t>(-1);
                  break;
                }
                acc = acc * 10 + static_cast<size_t>(s[k] - '0');
              }
              if (acc == static_cast<size_t>(-1))
                bad = true;
              else
                content_length = acc;
            }

            if (!bad) {
              // Check for Transfer-Encoding: chunked
              std::string str = c.req.getHeader("transfer-encoding");
              if (!str.empty()) {
                std::string v = str;
                for (size_t k = 0; k < v.size(); ++k)
                  v[k] = (char)std::tolower((unsigned char)v[k]);
                if (v.find("chunked") != std::string::npos)
                  has_te_chunked = true;
              }
            }

            // Having both CL and TE:chunked is invalid
            if (has_cl && has_te_chunked)
              bad = true;

            if (bad) {
              // Malformed or conflicint headers
              c.res.setStatusFromCode(400);
              c.res.setVersion(c.req.version);
              c.state = WRITING_RESPONSE;
              c.in.erase(0, endpos);
            } else {
              c.is_chunked = has_te_chunked;
              c.want_body = has_cl ? content_length : 0;

              // Match location once for max_size checks
              c.loc = Router::matchLocation(c.cfg, c.req.target);
              size_t maxSize =
                  c.loc ? c.loc->max_size : c.cfg.locations[0].max_size;

              if (c.want_body > 0 || c.is_chunked) {
                std::stringstream ss;
                ss << "/tmp/webserv_temp_" << fd << "_" << now_ms();
                c.temp_filename = ss.str();

                c.temp_fd = open(c.temp_filename.c_str(),
                                 O_CREAT | O_WRONLY | O_TRUNC, 0644);
                Logger::info("Created temp_fd: %s", c.temp_filename.c_str());
                if (c.temp_fd < 0) {
                  Logger::error("temp_fd return: %d, name: %s", c.temp_fd,
                                c.temp_filename.c_str());
                  c.res.setStatusFromCode(500);
                  c.state = WRITING_RESPONSE;
                  return;
                }
              }

              // size limit for non-chunked CL bodies
              if (has_cl && c.want_body > maxSize) {
                // Content-Length exceeds max_size:
                // respond 413 and stop processing
                Logger::request("%sbody size %s%s%s exceeds "
                                "max %s%s%s - responding 413",
                                GREY, RED, bytesToStr(c.want_body).c_str(),
                                GREY, YELLOW, bytesToStr(maxSize).c_str(),
                                GREY);
                c.res.setStatusFromCode(413);
                c.res.setVersion(c.req.version);
                c.state = WRITING_RESPONSE;
                // Drop the parsed headers
                // and any pending
                // input/body to avoid
                // reprocessing
                c.in.clear();
                c.body.clear();
                c.want_body = 0;
                c.is_chunked = false;
                c.close_after = true;
              } else {
                // Remove head so that only the body
                // is leftover
                c.in.erase(0, endpos);

                // Pre-consume body bytes for
                // Content-Length requests
                if (!c.is_chunked && c.want_body > 0 && !c.in.empty()) {
                  size_t take =
                      (c.in.size() > c.want_body) ? c.want_body : c.in.size();

                  if (c.temp_fd != -1) {
                    ssize_t written = write(c.temp_fd, c.in.data(), take);
                    if (written < 0) {
                      // ERROR 500
                    }
                    c.body_bytes_read += static_cast<size_t>(written);
                  } else {
                    c.body.append(c.in.data(), take);
                  }
                  c.in.erase(0, take);
                }

                if (c.is_chunked) {
                  // Initialize chunk decoder
                  c.decoder.reset();
                  c.state = READING_BODY;
                } else if (c.temp_fd != -1 &&
                           c.body_bytes_read >= c.want_body) {
                  close(c.temp_fd);
                  c.temp_fd = -1;
                  c.state = WRITING_RESPONSE;
                } else if (c.temp_fd == -1 && c.body.size() >= c.want_body) {
                  c.state = WRITING_RESPONSE;
                } else // still need more body bytes
                  c.state = READING_BODY;
              }
            }
          }
        }
      }

      // READING_BODY
      if (c.state == READING_BODY) {
        size_t decode_left = MAX_DECODE_BYTES;

        if (c.is_chunked) {
          // Chunked transfer decoding loop
          while (decode_left > 0 && !c.in.empty()) {
            size_t before = c.in.size();
            ChunkedDecoder::Status st = c.decoder.feed(c.in, c.body);
            size_t consumed = before - c.in.size();

            if (consumed > decode_left)
              decode_left = 0;
            else
              decode_left -= consumed;

            if (c.temp_fd != -1 && !c.body.empty()) {
              ssize_t written = write(c.temp_fd, c.body.data(), c.body.size());
              if (written < 0) {
                close(c.temp_fd);
                c.temp_fd = -1;
                c.res.setStatusFromCode(500);
                c.state = WRITING_RESPONSE;
                break;
              }
              c.body_bytes_read += static_cast<size_t>(written);
              c.body.clear();
            }
            if (st == ChunkedDecoder::NEED_MORE)
              break;
            if (st == ChunkedDecoder::ERROR) {
              c.res.setStatusFromCode(400);
              c.state = WRITING_RESPONSE;
              break;
            }
            if (st == ChunkedDecoder::DONE) {
              // Final size check after full decoding
              size_t maxSize =
                  c.loc ? c.loc->max_size : c.cfg.locations[0].max_size;

              size_t totalSize = c.body_bytes_read + c.body.size();
              if (totalSize > maxSize) {
                // Body exceeds max_size: respond
                // 413 and stop processing
                Logger::request("%schunked body size %s%s%s "
                                "exceeds max %s%s%s - "
                                "responding 413",
                                GREY, RED, bytesToStr(c.body.size()).c_str(),
                                GREY, YELLOW, bytesToStr(maxSize).c_str(),
                                GREY);
                c.res.setStatusFromCode(413);
                c.state = WRITING_RESPONSE;
                c.in.clear();
                c.want_body = 0;
                c.is_chunked = false;
                c.close_after = true;
              } else {
                c.state = WRITING_RESPONSE; // Full
                                            // request
                                            // body
                                            // decoded
              }
              if (c.temp_fd != -1) {
                close(c.temp_fd);
                c.temp_fd = -1;
              }
              break;
            }
            if (decode_left == 0)
              break;
          }
        } else {
          // Non-chunked body: consume up to want_body, capped by
          // decode_left
          if (c.want_body > 0 && !c.in.empty() && decode_left > 0) {
            size_t bytes_needed = c.want_body - c.body_bytes_read;
            size_t take = c.in.size();
            if (take > bytes_needed)
              take = bytes_needed;
            if (take > decode_left)
              take = decode_left;

            if (c.temp_fd != -1) {
              ssize_t written = write(c.temp_fd, c.in.data(), take);
              if (written < 0) {
                close(c.temp_fd);
                c.temp_fd = -1;
                c.res.setStatusFromCode(500);
                c.state = WRITING_RESPONSE;
                return;
              }
              c.body_bytes_read += static_cast<size_t>(written);
            } else {
              c.body.append(c.in.data(), take);
            }
            c.in.erase(0, take);
            decode_left -= take;
          }

          bool is_full = false;
          if (c.temp_fd != -1) {
            if (c.body_bytes_read >= c.want_body)
              is_full = true;
            else if (c.body.size() >= c.want_body)
              is_full = true;
          }

          if (is_full) {
            if (c.temp_fd != -1) {
              close(c.temp_fd);
              c.temp_fd = -1;
            }
            c.state = WRITING_RESPONSE;
          }
        }
      }

      // WRITING_RESPONSE
      if (c.state == WRITING_RESPONSE) {
        if (c.out.empty()) {
          prepareResponse(fd, c);

          if (c.state == WAITING_CGI)
            break;

          c.res.printResponse(fd);
        }
      }

      // Continue reading (if handled < MAX_HANDLE_BYTES) or exit the loop when
      // the budget is used up
      continue;
    }
    if (r == 0) {
      c.peer_closed = true;
      break;
    }
    break;
  }
}

void Server::handleWritable(int fd) {
  // Find the connection associated with the fd
  std::map<int, Connection>::iterator it = conns_.find(fd);
  if (it == conns_.end())
    return;

  Connection &c = it->second;

  c.last_active = now_ms();
  // Max number of bytes we will attempt to send in a single call.
  // This prevents one big response from blocking other clients.
  const size_t MAX_WRITE_BYTES = 16 * 1024;

  if (!c.out.empty() && !c.responded) {
    size_t written = 0; // bytes sent in this iteration
    ssize_t offset = 0; // bytes consumed from front of c.out

    // Socket write loop
    while (written < MAX_WRITE_BYTES &&
           offset < static_cast<ssize_t>(c.out.size())) {
      // Try to send as much as possible from c.out, starting at 'offset'.
      ssize_t w =
          ::send(fd, c.out.data() + offset,
                 c.out.size() - static_cast<size_t>(offset), MSG_NOSIGNAL);
      if (w > 0) {
        offset += w;
        written += static_cast<size_t>(w);
      } else {
        // w < 0 -> error
        // stop writing until epoll activates again
        break;
      }
    }

    // Drop the bytes we successfully sent from the front of c.out.
    if (offset > 0)
      c.out.erase(0, static_cast<size_t>(offset));
  }

  // If c.out is empty, decide to stream more file or finish
  if (c.out.empty()) {
    // If we still have a static file to stream, and we're not done
    if (c.streaming_file && c.file_fd >= 0 && c.file_remaining > 0 &&
        !c.responded) {
      // Read next chunk from the file into c.out
      const size_t FILE_CHUNK = 16 * 1024;

      char buf[FILE_CHUNK];
      // If streaming CGI output
      if (c.file_skip > 0) {
        size_t discard;
        if ((off_t)FILE_CHUNK > c.file_skip)
          discard = (size_t)c.file_skip;
        else
          discard = FILE_CHUNK;
        ssize_t skip = read(c.file_fd, buf, discard);
        if (skip > 0) {
          c.file_skip -= skip;
          return;
        } else {
          ::close(c.file_fd);
          c.file_fd = -1;
          c.streaming_file = false;
          c.file_remaining = 0;
          c.file_skip = 0;
          return;
        }
      }
      size_t to_read = FILE_CHUNK;
      if (static_cast<off_t>(to_read) > c.file_remaining)
        to_read = static_cast<size_t>(c.file_remaining);

      // Blocking read on regular file, but small (<= FILE_CHUNK) and not in a
      // loop
      ssize_t r = ::read(c.file_fd, buf, to_read);
      if (r > 0) {
        c.file_remaining -= static_cast<off_t>(r);
        c.out.append(buf, static_cast<size_t>(r));

        if (c.file_remaining <= 0) {
          if (!c.cgi_out_path.empty()) {
            unlink(c.cgi_out_path.c_str());
            c.cgi_out_path.clear();
          }
          ::close(c.file_fd);
          c.file_fd = -1;
          c.streaming_file = false;
        }

        // Not marked as responded or close here
        // just wait for epoll to call to send it.
        return;
      } else {
        ::close(c.file_fd);
        c.file_fd = -1;
        c.streaming_file = false;
        c.file_remaining = 0;
      }
    }

    if (c.state == WAITING_CGI)
      return;

    // No data left to send and no more file to stream
    if (!c.responded)
      c.responded = true;

    // Check if we should keep connection alive for HTTP/1.1 Keep-Alive
    if (shouldKeepConnectionAlive(c)) {
      Logger::connection("fd %d keeping connection alive", fd);
      if (!c.cgi_out_path.empty()) {
        unlink(c.cgi_out_path.c_str());
        c.cgi_out_path.clear();
      }
      c.resetForNextRequest();
      // Switch back to reading mode for next request
      disableWrite(fd);
      return;
    }

    // Connection should be closed
    Logger::connection("fd %d closed - connection removed", fd);
    if (!c.cgi_out_path.empty()) {
      unlink(c.cgi_out_path.c_str());
      c.cgi_out_path.clear();
    }
    cgiHandler_.detachConnection(&c);
    reactor_.del(fd);
    if (c.file_fd != -1) {
      ::close(c.file_fd);
      c.file_fd = -1;
    }
    ::close(fd);
    conns_.erase(it);
  }
}

int Server::executeStdin() {
  char buff[50];

  ssize_t sr = read(0, buff, sizeof(buff) - 1);
  if (sr <= 0)
    return false;
  buff[sr] = '\0';
  while (sr > 0 && (buff[sr - 1] == '\n' || buff[sr - 1] == '\r'))
    buff[--sr] = '\0';
  if (sr == 0)
    return false;
  if (std::strcmp(buff, "clear") == 0) {
    const char *clr = "\033[H\033[2J\033[3J";
    write(1, clr, std::strlen(clr));
    return false;
  } else if (std::strcmp(buff, "quit") == 0 || std::strcmp(buff, "q") == 0) {
    return (1);
  } else if (std::strcmp(buff, "conf") == 0) {
    conf_.debug_print();
  } else if (std::strcmp(buff, "r") == 0 || std::strcmp(buff, "refresh") == 0) {
    refresh();
  } else if (std::strcmp(buff, "buff") == 0) {
    std::cout.write(&inbuf_[0], 200);
    std::cout << std::endl;
  } else if (std::strcmp(buff, "list") == 0) {
    unsigned long n = conns_.size();
    Logger::timer("%u %sconnection%c", n, YELLOW, n > 1 ? 's' : ' ');
    for (std::map<int, Connection>::iterator it = conns_.begin();
         it != conns_.end(); ++it) {
      std::ostringstream oss;
      oss << PURPLE << " Conn	" << TS;
      std::string label = oss.str();
      it->second.printStatus(label);
    }
  } else if (std::strncmp(buff, "log", 3) == 0) {
    if (buff[3]) {
      std::string str = buff + 4;
      std::vector<std::string> arr = split(str, ' ');
      for (size_t j = 0; j < arr.size(); j++) {
        std::string level = arr[j];
        size_t x = 0;
        while (level[x] >= '0' && level[x] <= '9' && x < level.size()) {
          Logger::setChannel((LogChannel)(level[x] - '0'));
          x++;
        }
        if (x)
          continue;

        std::string uLevel = toUpper(level);
        for (int i = 0; i < LOG_ALL + 1; i++) {
          if (!std::strncmp(level.c_str(), LoggerLevels[i].c_str(), 3)) {
            Logger::setChannel(LOG_NONE);
            Logger::setChannel((LogChannel)i);
            break;
          }
          if (!std::strncmp(uLevel.c_str(), LoggerLevels[i].c_str(), 3)) {
            Logger::setChannel((LogChannel)i);
            break;
          }
        }
      }
    }
    Logger::printChannels();
  }
  return false;
}

void Server::refresh() {
  if (!conf_.parse(conf_path_)) {
    std::cerr << "Config error at line " << conf_.getErrorLine() << ": "
              << conf_.getErrorMessage() << std::endl;
    return;
  }
  conf_.debug_print();
  cleanup();
  if (!start()) {
    std::perror("webserv: start failed (is another instance running?");
    return;
  }
  Logger::server("Server refreshed");
}

int Server::run() {
  bool logged = false;
  reactor_.add(STDIN_FILENO, EPOLLIN);

  while (true) {
    epoll_event events[128];
    int n = reactor_.wait(events, 128, 1000);

    if (n > 0) {
      for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;
        if (fd == 0) {
          if (int run = executeStdin()) {
            cleanup();
            return (run);
          }
          logged = false;
          continue;
        }
        uint32_t ev = events[i].events;

        if (std::find(listener_.begin(), listener_.end(), fd) !=
            listener_.end()) {
          if (ev & EPOLLIN)
            acceptReady();
          continue;
        }

        std::map<int, Connection>::iterator it = conns_.find(fd);
        if (it != conns_.end()) {
          if (ev & (EPOLLHUP | EPOLLERR)) {
            cgiHandler_.detachConnection(&it->second);
            reactor_.del(fd);
            ::close(fd);
            conns_.erase(fd);
            continue;
          }
          if (ev & EPOLLIN)
            handleReadable(fd);
          if (ev & EPOLLOUT)
            handleWritable(fd);
          continue;
        }

        if (cgiHandler_.hasFd(fd)) {
          if (ev & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
            cgiHandler_.handleMessage(fd);
          }
          continue;
        }
      }
      if (logged)
        Logger::server("ready\n");
      logged = false;
    } else {
      if (!logged) {
        Logger::server("waiting...");
      }
      logged = true;
    }

    checkTimeouts();
    cgiHandler_.checkCgiTimeouts();
    //  cgiHandler_.handleResponses();

    for (std::map<int, Connection>::iterator it = conns_.begin();
         it != conns_.end(); ++it) {
      if (it->second.state == WRITING_RESPONSE && !it->second.out.empty())
        enableWrite(it->first);
    }
  }
}
