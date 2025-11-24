#include "ChunkedDecoder.hpp"

ChunkedDecoder::ChunkedDecoder() : state_(READ_SIZE_LINE), chunk_rem_(0) {}

void	ChunkedDecoder::reset()
{
	state_ = READ_SIZE_LINE;
	chunk_rem_ = 0;
}

bool	ChunkedDecoder::read_line(std::string& in, std::string& line)
{
	size_t	pos = in.find("\r\n");
	if (pos == std::string::npos)
		return (false);
	line.assign(in, 0, pos);
	in.erase(0, pos + 2);
	return (true);
}

bool	ChunkedDecoder::parse_size_line(const std::string& line, size_t& size)
{
	// chunk-size *(; chunk-ext). We ignore extensions after ';'
	size = 0;
	size_t	end = line.find(";");
	const std::string	hex = (end == std::string::npos) ? line : line.substr(0, end);
	if (hex.empty())
		return (false);
	
	const size_t	SIZE_T_MAX_VALUE = std::numeric_limits<size_t>::max();
	
	// Parse hex (no leading 0x)
	size_t	acc = 0;
	for (size_t i = 0; i < hex.size(); ++i)
	{
		unsigned char	c = (unsigned char)hex[i];
		unsigned		v;
		if (c >= '0' && c <= '9')
			v = c - '0';
		else if (c >= 'a' && c <= 'f')
			v = 10 + (c - 'a');
		else if (c >= 'A' && c <= 'F')
			v = 10 + (c - 'A');
		else
			return (false);
		// overflow check
		if (acc > (SIZE_T_MAX_VALUE >> 4))
			return (false);
		acc = (acc << 4) | v;
	}
	size = acc;
	return (true);
}

ChunkedDecoder::Status ChunkedDecoder::feed(std::string& in, std::string& out)
{
	for (;;)
	{
		switch (state_)
		{
			case (READ_SIZE_LINE):
			{
				std::string	line;
				if (!read_line(in, line))
					return (NEED_MORE);
				size_t	size = 0;
				if (!parse_size_line(line, size))
					return (ERROR);
				chunk_rem_ = size;
				if (chunk_rem_ == 0)
					state_ = READ_TRAILERS; // Next: trailers, until CRLFCRLF
				else
					state_ = READ_DATA;
				break;
			}
			case (READ_DATA):
			{
				if (in.size() < chunk_rem_)
				{
					out.append(in);
					chunk_rem_ -= in.size();
					in.clear();
					return (NEED_MORE);
				}
				else
				{
					out.append(in, 0, chunk_rem_);
					in.erase(0, chunk_rem_);
					chunk_rem_ = 0;
					state_ = READ_DATA_CRLF;
				}
				break;
			}
			case (READ_DATA_CRLF):
			{
				if (in.size() < 2)
					return (NEED_MORE);
				if (in[0] != '\r' || in[1] != '\n')
					return (ERROR);
				in.erase(0, 2);
				state_ = READ_SIZE_LINE;
				break;
			}
			case (READ_TRAILERS):
			{
				// trailers end with an empty line (CRLF).
				std::string	line;
				if (!read_line(in, line))
					return (NEED_MORE);
				if (line.empty())
				{
					state_ = FINISHED;
					return (DONE);
				}
				// ignore trailer header lines
				break;
			}
			case (FINISHED):
				return (DONE);
		}
	}
}
