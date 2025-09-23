#pragma once
#include <string>
#include <map>
#include "../http/HttpRequest.hpp"
#include "ChunkedDecoder.hpp"

enum ConnState {
	READING_HEADERS,
	READING_BODY,
	WRITING_RESPONSE
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

	Connection()
	: headers_done(false), responded(false), peer_closed(false),
	close_after(false), state(READING_HEADERS), want_body(0),
	is_chunked(false), has_req(false) {}
};
