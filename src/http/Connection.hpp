#pragma once
#include <string>
#include <ctime>
#include <sys/types.h>
#include "../http/HttpRequest.hpp"
#include "../utils/Chrono.hpp"
#include "../utils/Logger.hpp"
#include "ChunkedDecoder.hpp"
#include <ctime>
#include <map>
#include <string>

enum ConnState {
	READING_HEADERS,
	READING_BODY,
	READY_TO_RESPOND,
	WRITING_RESPONSE,
	CLOSING
};

class Connection {
    public:
	std::string in;	  // head + maybe more
	std::string out;  // response bytes to send
	std::string body; // request body as we accumulate it

	bool headers_done;
	bool responded;
	bool peer_closed;
	bool close_after;

	ConnState state;
	size_t want_body; // expected body length (from Content-Length)
	bool is_chunked;  // trye if TE: chunked

	HttpRequest req;
	bool has_req;

	ChunkedDecoder decoder;

	// static file streaming state
	int				file_fd;		// fd of file being streamed, -1 if none
	off_t			file_remaining;	// bytes left to send
	bool			streaming_file;	// true if we still need to stream body from file
	time_t last_active; // for idle timeout

	void printStatus(const std::string &label) {
		Logger::simple("%s - in: %s out: %s last: %s state: %d", label.c_str(), in.c_str(),
			       out.c_str(), formatTime(last_active).c_str(), (int)state);
	}
	Connection()
	    : headers_done(false), responded(false), peer_closed(false), close_after(false),
	      state(READING_HEADERS), want_body(0), is_chunked(false), has_req(false),
	      file_fd(-1), file_remaining(0), streaming_file(false) {}
};
