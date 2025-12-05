#pragma once
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "ChunkedDecoder.hpp"
#include "HttpResponse.hpp"
#include <ctime>
#include <string>
#include <sys/types.h>

enum ConnState {
  READING_HEADERS,
  READING_BODY,
  READY_TO_RESPOND,
  WRITING_RESPONSE,
  CLOSING
};

class Connection {
public:
  std::string in;   // head + maybe more
  std::string body; // request body as we accumulate it
  std::string out;  // response bytes to send

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
  int serverIdx; // index for different server confs

  ChunkedDecoder decoder;

  // static file streaming state
  int file_fd;          // fd of file being streamed, -1 if none
  off_t file_remaining; // bytes left to send
  bool streaming_file;  // true if we still need to stream body from file
  time_t last_active;   // for idle timeout

  void printStatus(const std::string &label);
  Connection(void);
};
