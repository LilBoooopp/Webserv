#pragma once
#include <string>
#include <cstddef>
#include <cerrno>
#include <limits>

class ChunkedDecoder {
public:
	enum Status { NEED_MORE, DONE, ERROR };
	ChunkedDecoder();

	// Consumes bytes from 'in' and appends decoded data to 'out'.
	// Returns NEED_MORE (keep feeding), DONE (reached 0-size chunk and finished trailers),
	// or ERROR (malformed).
	Status	feed(std::string& in, std::string& out);

	void	reset();
private:
	enum State { READ_SIZE_LINE, READ_DATA, READ_DATA_CRLF, READ_TRAILERS, FINISHED };
	State	state_;
	size_t	chunk_rem_; // remaining bytes in current chunk

	bool	read_line(std::string& in, std::string& line); // reads up to CRLF, removes it, false if not present.
	bool	parse_size_line(const std::string& line, size_t& size); // hex, ignore ";ext"
};
