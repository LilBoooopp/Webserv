#include "Connection.hpp"

Connection::Connection(const ServerConf &cfg)
    : headers_done(false), responded(false), peer_closed(false),
      close_after(false), state(READING_HEADERS), want_body(0),
      is_chunked(false), temp_fd(-1), res(200), file_fd(-1), file_remaining(0),
      streaming_file(false), cfg(cfg), loc(NULL) {
  last_active = std::time(NULL);
}

Connection &Connection::operator=(const Connection &other) {
  if (this != &other) {
    // Copy all members except cfg (const reference)
    in = other.in;
    body = other.body;
    out = other.out;
    id = other.id;
    user = other.user;
    password = other.password;
    isAuthentified = other.isAuthentified;
    headers_done = other.headers_done;
    responded = other.responded;
    peer_closed = other.peer_closed;
    close_after = other.close_after;
    state = other.state;
    want_body = other.want_body;
    is_chunked = other.is_chunked;
    start = other.start;
    req = other.req;
    res = other.res;
    decoder = other.decoder;
    file_fd = other.file_fd;
    file_remaining = other.file_remaining;
    streaming_file = other.streaming_file;
    last_active = other.last_active;
    loc = other.loc;
    // cfg remains unchanged - it's a const reference to the original
  }
  return *this;
}

void Connection::printStatus(const std::string &label) {
  std::string ConnStateStr[CLOSING + 1] = {
      "READING_HEADER",    "READING_BODY", "READY_TO_RESPOND",
      "WRITTING_RESPONSE", "WAITING",      "CLOSING"};
  Logger::simple("%s - in: %lu bytes, out: %lu bytes, last: %s state: %s [%d]",
                 label.c_str(), (unsigned long)in.size(),
                 (unsigned long)out.size(), formatTime(last_active).c_str(),
                 ConnStateStr[(int)state].c_str(), (int)state);
}
