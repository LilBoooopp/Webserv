#pragma once
#include <string>
#include <map>
#include <ctime>
#include "../http/HttpRequest.hpp"
#include "ChunkedDecoder.hpp"
#include "../utils/TimerQueue.hpp"

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

	time_t			last_active; // for idle timeout

	u64				gen_header;
	u64				gen_body;
	u64				gen_send;
	u64				gen_keep;

	Connection()
	: headers_done(false), responded(false), peer_closed(false),
	close_after(false), state(READING_HEADERS), want_body(0),
	is_chunked(false), has_req(false), gen_header(0),
	gen_body(0), gen_send(0), gen_keep(0),
	last_active(std::time(NULL)) {}
};
