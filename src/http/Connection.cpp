#include "Connection.hpp"
#include "../utils/Chrono.hpp"
#include "../utils/Logger.hpp"
#include "HttpResponse.hpp"
#include <string>

Connection::Connection(void)
    : headers_done(false), responded(false), peer_closed(false),
      close_after(false), state(READING_HEADERS), want_body(0),
      is_chunked(false), res(200), serverIdx(-1), file_fd(-1),
      file_remaining(0), streaming_file(false) {}

void Connection::printStatus(const std::string &label) {
  Logger::simple("%s - in: %s out: %s last: %s state: %d", label.c_str(),
                 in.c_str(), out.c_str(), formatTime(last_active).c_str(),
                 (int)state);
}
