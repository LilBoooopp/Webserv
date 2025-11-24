#pragma once
#include <string>
#include <ctime>
#include "../http/HttpRequest.hpp"
#include "ChunkedDecoder.hpp"

enum ConnState {
	READING_HEADERS,
	READING_BODY,
	READY_TO_RESPOND,
	WRITING_RESPONSE,
	CLOSING
};

class Connection {
public:
	std::string		in;		// head + maybe more
	std::string		out;	// response bytes to send
	std::string		body;	// request body as we accumulate it

	bool			headers_done;
	bool			responded;
	bool			peer_closed;
	bool			close_after;

	ConnState		state;
	size_t			want_body;	// expected body length (from Content-Length)
	bool			is_chunked;	// trye if TE: chunked

	HttpRequest		req;
	bool			has_req;

	ChunkedDecoder	decoder;

	time_t			last_active; // for idle timeout

	Connection()
	: headers_done(false), responded(false), peer_closed(false),
	close_after(false), state(READING_HEADERS), want_body(0),
	is_chunked(false), has_req(false), last_active(std::time(NULL)) {}
};
