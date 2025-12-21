#pragma once
#include "../config/Config.hpp"
#include "../utils/Utils.hpp"
#include "ChunkedDecoder.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <ctime>
#include <string>
#include <sys/types.h>

enum ConnState {
  READING_HEADERS,
  READING_BODY,
  READY_TO_RESPOND,
  WAITING_CGI,
  WRITING_RESPONSE,
  WAITING,
  CLOSING
};

class Connection {
public:
  std::string in;   // head + maybe more
  std::string body; // request body as we accumulate it
  std::string out;  // response bytes to send
  std::string
      id; // session id stored in browser after /login, used to id requests
  std::string user;
  std::string password;
  bool isAuthentified;

  bool headers_done;
  bool responded;
  bool peer_closed;
  bool close_after;
  ConnState state;
  size_t want_body; // expected body length (from Content-Length)
  bool is_chunked;  // trye if TE: chunked

  size_t start;
  HttpRequest req;

  HttpResponse res;

  ChunkedDecoder decoder;

  // static file streaming state
  int file_fd;               // fd of file being streamed, -1 if none
  off_t file_remaining;      // bytes left to send
  bool streaming_file;       // true if we still need to stream body from file
  unsigned long last_active; // for idle timeout
  const ServerConf &cfg;
  const LocationConf *loc;
  Connection(const ServerConf &cfg);
  Connection &operator=(const Connection &other);
  void printStatus(const std::string &label);
};
