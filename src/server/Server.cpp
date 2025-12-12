#include "Server.hpp"
#include "../http/ChunkedDecoder.hpp"
#include "../http/HttpParser.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/Chrono.hpp"
#include "../utils/Colors.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Path.hpp"
#include "IHandler.hpp"
#include "Listener.hpp"
#include "Router.hpp"
#include <algorithm>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

void Server::enableWrite(int fd) { reactor_.mod(fd, EPOLLIN | EPOLLOUT); }
void Server::disableWrite(int fd) { reactor_.mod(fd, EPOLLIN); }

static int set_nonblock(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	return ((flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0) ? 0 : -1);
}

/**
 * @brief Starts the server by bind and listening on the host and ports
 *
 * @param ip_be
 * @param port_be
 * @param config
 * @return true if started successfully, false if not
 */
bool Server::start(std::vector<ServerConf> &config) {
	listener_.reserve(config.size());
	uint32_t ip_be = config[0].hosts[0].host;
	for (size_t i = 0; i < config.size(); i++) {
		listener_.push_back(Listener());
		if (!listener_[i].bindAndListen(ip_be, config[i].hosts[0].port))
			return (false);
		if (!reactor_.add(listener_[i].fd(), EPOLLIN))
			return (false);
	}
	cgiHandler_.setConfig(config);

	return (true);
}

void Server::setConf(std::vector<ServerConf> config) {
	cfg_ = config;
	std::cout << "index: " << cfg_[0].locations[0].index_files[0] << std::endl;
}

/**
 * @brief Assigns incomming connections' fds to the previously bound listeners
 */
void Server::acceptReady(void) {
	for (size_t i = 0; i < listener_.size(); i++) {
		if (listener_[i].fd() < 0) {
			Logger::connection("unvalid fd %d", listener_[i].fd());
			return;
		}
		for (;;) {
			int cfd = ::accept(listener_[i].fd(), 0, 0);
			if (cfd < 0) {
				break;
			}
			set_nonblock(cfd);
			Connection c;
			c.start = now_ms();
			c.serverIdx = i;
			conns_[cfd] = c;
			reactor_.add(cfd, EPOLLIN);
			Logger::connection("fd %d%s accepted", cfd, GREEN);
		}
	}
}

/**
 * @brief Reads the bytes sent by the client as a whole or chunked if necessary
 *
 * @param fd of the socket associated with the current client being handled
 */
void Server::handleReadable(int fd) {
	Connection &c = conns_[fd];

	const size_t MAX_HANDLE_BYTES = 16 * 1024; // max bytes to read from socket per iteration
	const size_t MAX_DECODE_BYTES =
	    16 * 1024; // max bytes to consume from c.in for body parsin per iteration

	size_t handled = 0;
	while (handled < MAX_HANDLE_BYTES) {
		ssize_t r = ::read(fd, &inbuf_[0], inbuf_.size());
		if (r > 0) {
			handled += static_cast<size_t>(r);
			c.in.append(&inbuf_[0], static_cast<size_t>(r));

			// Header cap defense
			// if (c.state == READING_HEADERS && c.in.size() > MAX_HANDLE_BYTES)
			//{
			//	HttpResponse	res(431);
			//	c.res.setContentType("text/plain");
			//	c.res.setBody("header too large");
			//	c.out = c.res.serialize(false);
			//	c.state = WRITING_c.resPONSE;
			//	enableWrite(fd);
			//	c.in.clear();
			//	break;
			//}

			// READING_HEADERS
			if (c.state == READING_HEADERS) {
				size_t eoh = c.in.find("\r\n\r\n");
				if (eoh != std::string::npos) {
					c.headers_done = true;

					size_t endpos = 0;

					if (!HttpParser::parse(c, endpos)) {
						if (c.req.version != "HTTP/1.1" &&
						    c.req.version != "HTTP/1.0" &&
						    c.req.method != "" && c.req.target != "") {
							c.res.setStatusFromCode(505);
						} else {
							c.res.setStatusFromCode(400);
							c.res.setVersion(c.req.version);
						}
						c.state = WRITING_RESPONSE;
						c.in.clear();
					} else {
						bool bad = false;

						// TE/CL detection
						bool has_te_chunked = false;
						bool has_cl = false;
						size_t content_length = 0;

						// Check for Content-Length header
						std::map<std::string, std::string>::iterator itCL =
						    c.req.headers.find("content-length");
						if (itCL != c.req.headers.end() && !bad) {
							has_cl = true;
							const std::string &s = itCL->second;
							size_t acc = 0;
							for (size_t k = 0; k < s.size(); ++k) {
								if (s[k] < '0' || s[k] > '9') {
									acc =
									    static_cast<size_t>(-1);
									break;
								}
								acc =
								    acc * 10 +
								    static_cast<size_t>(s[k] - '0');
							}
							if (acc == static_cast<size_t>(-1))
								bad = true;
							else
								content_length = acc;
						}

						// Check for Transfer-Encoding: chunked
						std::map<std::string, std::string>::iterator itTE =
						    c.req.headers.find("transfer-encoding");
						if (itTE != c.req.headers.end() && !bad) {
							std::string v = itTE->second;
							for (size_t k = 0; k < v.size(); ++k)
								v[k] = (char)std::tolower(
								    (unsigned char)v[k]);
							if (v.find("chunked") != std::string::npos)
								has_te_chunked = true;
						}

						// Having both CL and TE:chunked is invalid
						if (has_cl && has_te_chunked)
							bad = true;

						if (bad) {
							// Malformed or conflicint headers
							c.res.setStatusFromCode(400);
							c.res.setVersion(c.req.version);
							c.state = WRITING_RESPONSE;
							c.in.erase(0, endpos);
						} else {
							c.is_chunked = has_te_chunked;
							c.want_body = has_cl ? content_length : 0;
							c.body.clear();

							// size limit for non-chunked CL bodies
							if (has_cl &&
							    c.want_body > cfg_[c.serverIdx]
									      .locations[0]
									      .max_size) {
								// Content-Length exceeds max_size:
								// respond 413 and stop processing
								c.res.setStatusFromCode(413);
								c.res.setVersion(c.req.version);
								c.state = WRITING_RESPONSE;
								// Drop the parsed headers and any
								// pending input/body to avoid
								// reprocessing
								c.in.clear();
								c.body.clear();
								c.want_body = 0;
								c.is_chunked = false;
								c.close_after = true;
							} else {
								// Remove head so that only the body
								// is leftover
								c.in.erase(0, endpos);

								// Pre-consume body bytes for
								// Content-Length requests
								if (!c.is_chunked &&
								    c.want_body > 0 &&
								    !c.in.empty()) {
									size_t take =
									    (c.in.size() >
									     c.want_body)
										? c.want_body
										: c.in.size();
									c.body.append(c.in.data(),
										      take);
									c.in.erase(0, take);
								}

								if (c.is_chunked) {
									// Initialize chunk decoder
									c.decoder.reset();
									c.state = READING_BODY;
								} else if (c.want_body ==
									   c.body.size()) // No Body
											  // or
											  // already
											  // have
											  // entire
											  // body
									c.state = WRITING_RESPONSE;
								else // still need more body bytes
									c.state = READING_BODY;
							}
						}
					}
				}
			}

			// READING_BODY
			if (c.state == READING_BODY) {
				size_t decode_left = MAX_DECODE_BYTES;

				if (c.is_chunked) {
					// Chunked transfer decoding loop
					while (decode_left > 0 && !c.in.empty()) {
						size_t before = c.in.size();
						ChunkedDecoder::Status st =
						    c.decoder.feed(c.in, c.body);
						size_t consumed = before - c.in.size();
						if (consumed > decode_left)
							consumed = decode_left;
						decode_left -= consumed;

						if (st == ChunkedDecoder::NEED_MORE)
							break;
						if (st == ChunkedDecoder::ERROR) {
							c.res.setStatusFromCode(400);
							c.state = WRITING_RESPONSE;
							break;
						}
						if (st == ChunkedDecoder::DONE) {
							// Final size check after full decoding
							if (c.body.size() > cfg_[c.serverIdx]
										.locations[0]
										.max_size) {
								// Body exceeds max_size: respond
								// 413 and stop processing
								c.res.setStatusFromCode(413);
								c.state = WRITING_RESPONSE;
								c.in.clear();
								c.want_body = 0;
								c.is_chunked = false;
								c.close_after = true;
							} else {
								c.state =
								    WRITING_RESPONSE; // Full
										      // request
										      // body
										      // decoded
							}
							break;
						}
						if (decode_left == 0)
							break;
					}
				} else {
					// Non-chunked body: consume up to want_body, capped by
					// decode_left
					if (c.want_body > c.body.size() && !c.in.empty() &&
					    decode_left > 0) {
						size_t room = c.want_body - c.body.size();
						size_t take = c.in.size();
						if (take > room)
							take = room;
						if (take > decode_left)
							take = decode_left;

						c.body.append(c.in.data(), take);
						c.in.erase(0, take);
						decode_left -= take;
					}
					// If we now have the full body, we can move on to
					// responding
					if (c.body.size() == c.want_body)
						c.state = WRITING_RESPONSE;
				}
			}

			// WRITING_RESPONSE
			if (c.state == WRITING_RESPONSE) {
				if (c.out.empty()) {
					prepareResponse(fd, c);
					c.res.printResponse(fd);
				}
			}

			// Continue reading (if handled < MAX_HANDLE_BYTES) or exit the loop when
			// the budget is used up
			continue;
		}
		if (r == 0) {
			c.peer_closed = true;
			break;
		}
		break;
	}
}

// Build and serialize a response once the request/body are ready
void Server::prepareResponse(int fd, Connection &c) {
	const HttpRequest &req = c.req;
	c.res.setVersion(req.version);

	Router router(cfg_[c.serverIdx]);

	if (c.res.getStatus() != 200) {
		router.redirectError(c);
		c.out = c.res.serialize(req.method == "HEAD");
		enableWrite(fd);
		return;
	}
	logRequest(req);

	IHandler *handler = router.route(c, req, c.res);

	std::string redirect_target = Router::getRedirectTarget(handler);
	if (!redirect_target.empty() && is_cgi(redirect_target, cfg_[c.serverIdx])) {
		c.req.target = redirect_target;
		delete handler;
		if (cgiHandler_.runCgi(c.req, c.res, c, fd))
			return;
		c.res.setStatusFromCode(500);
	} else {
		handler->handle(c, req, c.res);
		delete handler;
		if (is_cgi(req.target, cfg_[c.serverIdx])) {
			if (cgiHandler_.runCgi(req, c.res, c, fd))
				return;
		}
	}

	// Detect large static file streaming case for GET
	if (req.method == "GET") {
		const std::string kStreamHeader = "X-Stream-File";

		if (c.res.hasHeader(kStreamHeader)) {
			std::string file_path = c.res.getHeader(kStreamHeader);

			// Try to open the file now, in non-streaming, blocking mode.
			// We will only read it in small chunks later form handleWritable.
			int ffd = ::open(file_path.c_str(), O_RDONLY);
			if (ffd >= 0) {
				c.file_fd = ffd;
				c.streaming_file = true;

				// Initialize remaining bytes from Content-Length
				std::string cl = c.res.getHeader("Content-Length");
				off_t remaining = 0;
				if (!cl.empty()) {
					std::istringstream iss(cl);
					iss >> remaining;
				}
				c.file_remaining = remaining;

				// Remove the internal header so the client doesn't see it.
				c.res.eraseHeader(kStreamHeader);
			} else {
				// If open fails, fall back to a 404
				c.res.setStatusFromCode(404);

				// ensure no streaming
				c.streaming_file = false;
				c.file_fd = -1;
				c.file_remaining = 0;
			}
		}
	}
	else
		c.res.setStatusFromCode(405); // Method Not Allowed

	if (c.res.getStatus() >= 400)
		router.redirectError(c);
	c.out = c.res.serialize(req.method == "HEAD");
	enableWrite(fd);
}

void Server::handleWritable(int fd) {
	// Find the connection associated with the fd
	std::map<int, Connection>::iterator it = conns_.find(fd);
	if (it == conns_.end())
		return;

	Connection &c = it->second;
	// Max number of bytes we will attempt to send in a single call.
	// This prevents one big response from blocking other clients.
	const size_t MAX_WRITE_BYTES = 16 * 1024;

	if (!c.out.empty() && !c.responded) {
		size_t written = 0; // bytes sent in this iteration
		ssize_t offset = 0; // bytes consumed from front of c.out

		// Socket write loop
		while (written < MAX_WRITE_BYTES && offset < static_cast<ssize_t>(c.out.size())) {
			// Try to send as much as possible from c.out, starting at 'offset'.
			ssize_t w =
			    ::send(fd, c.out.data() + offset,
				   c.out.size() - static_cast<size_t>(offset), MSG_NOSIGNAL);
			if (w > 0) {
				offset += w;
				written += static_cast<size_t>(w);
			} else {
				// w < 0 -> error
				// stop writing until epoll activates again
				break;
			}
		}

		// Drop the bytes we successfully sent from the front of c.out.
		if (offset > 0)
			c.out.erase(0, static_cast<size_t>(offset));
	}

	// If c.out is empty, decide to stream more file or finish
	if (c.out.empty()) {
		// If we still have a static file to stream, and we're not done
		if (c.streaming_file && c.file_fd >= 0 && c.file_remaining > 0 && !c.responded) {
			// Read next chunk from the file into c.out
			const size_t FILE_CHUNK = 16 * 1024;

			char buf[FILE_CHUNK];
			size_t to_read = FILE_CHUNK;
			if (static_cast<off_t>(to_read) > c.file_remaining)
				to_read = static_cast<size_t>(c.file_remaining);

			// Blocking read on regular file, but small (<= FILE_CHUNK) and not in a
			// loop
			ssize_t r = ::read(c.file_fd, buf, to_read);
			if (r > 0) {
				c.file_remaining -= static_cast<off_t>(r);
				c.out.append(buf, static_cast<size_t>(r));
				// Not marked as responded or close here
				// just wait for epoll to call to send it.
				return;
			} else {
				::close(c.file_fd);
				c.file_fd = -1;
				c.streaming_file = false;
				c.file_remaining = 0;
			}
		}

		// No data left to send an no more file to stream
		if (!c.responded)
			c.responded = true;
		Logger::connection("fd %d closed", fd);

		cgiHandler_.detachConnection(&c);
		reactor_.del(fd);
		::close(fd);
		conns_.erase(it);
	}
}

bool Server::executeStdin() {
	char buff[50];

	ssize_t sr = read(0, buff, sizeof(buff) - 1);
	if (sr <= 0)
		return false;
	buff[sr] = '\0';
	while (sr > 0 && (buff[sr - 1] == '\n' || buff[sr - 1] == '\r'))
		buff[--sr] = '\0';
	if (sr == 0)
		return false;
	if (std::strcmp(buff, "clear") == 0) {
		const char *clr = "\033[H\033[2J\033[3J";
		write(1, clr, std::strlen(clr));
		return false;
	} else if (std::strcmp(buff, "quit") == 0) {
		return true;
	} else if (std::strcmp(buff, "buff") == 0) {
		std::cout.write(&inbuf_[0], 200);
		std::cout << std::endl;
	} else if (std::strcmp(buff, "list") == 0) {
		unsigned long n = conns_.size();
		Logger::timer("%u %sconnection%c", n, YELLOW, n > 1 ? 's' : ' ');
		for (unsigned long i = 0; i < n; i++) {
			std::ostringstream oss;
			oss << PURPLE << " Conn[" << i << "]" << TS;
			std::string label = oss.str();
			conns_[i].printStatus(label);
		}
	} else if (std::strncmp(buff, "log", 3) == 0) {
		if (buff[3]) {
			std::string str = buff + 4;
			std::vector<std::string> arr;
			split(str, ' ', arr);
			for (size_t j = 0; j < arr.size(); j++) {
				std::string level = arr[j];
				size_t x = 0;
				while (level[x] >= '0' && level[x] <= '9' && x < level.size()) {
					Logger::setChannel((LogChannel)(level[x] - '0'));
					x++;
				}
				if (x)
					continue;

				std::string uLevel = toUpper(level);
				for (int i = 0; i < LOG_ALL + 1; i++) {
					if (!std::strncmp(level.c_str(), LoggerLevels[i].c_str(),
							  3)) {
						Logger::setChannel(LOG_NONE);
						Logger::setChannel((LogChannel)i);
						break;
					}
					if (!std::strncmp(uLevel.c_str(), LoggerLevels[i].c_str(),
							  3)) {
						Logger::setChannel((LogChannel)i);
						break;
					}
				}
			}
		}
		Logger::printChannels();
	}
	return false;
}

void Server::run() {
	bool logged = false;
	reactor_.add(STDIN_FILENO, EPOLLIN);

	while (true) {
		epoll_event events[64];
		int n = reactor_.wait(events, 64, 1);

		if (n > 0) {
			for (int i = 0; i < n; ++i) {
				int fd = events[i].data.fd;
				if (fd == 0) {
					if (executeStdin()) {
						cleanup();
						return;
					}
					logged = false;
					continue;
				}
				uint32_t ev = events[i].events;

				if (std::find(listener_.begin(), listener_.end(), fd) !=
				    listener_.end()) {
					if (ev & EPOLLIN)
						acceptReady();
					continue;
				}

				if (ev & (EPOLLHUP | EPOLLERR)) {
					std::map<int, Connection>::iterator it = conns_.find(fd);
					if (it != conns_.end())
						cgiHandler_.detachConnection(&it->second);
					reactor_.del(fd);
					::close(fd);
					conns_.erase(fd);
					continue;
				}
				if (ev & EPOLLIN)
					handleReadable(fd);
				if (ev & EPOLLOUT)
					handleWritable(fd);
			}
			if (logged)
				Logger::server("ready\n");
			logged = false;
		} else {
			if (!logged) {
				Logger::server("waiting...");
			}
			logged = true;
		}

		cgiHandler_.handleResponses();

		for (std::map<int, Connection>::iterator it = conns_.begin(); it != conns_.end();
		     ++it) {
			if (it->second.state == WRITING_RESPONSE && !it->second.out.empty())
				enableWrite(it->first);
		}
	}
}

void Server::cleanup() {
	Logger::server("Cleaning up server resources...");
	cgiHandler_.killAsyncProcesses();
	for (std::map<int, Connection>::iterator it = conns_.begin(); it != conns_.end(); ++it) {
		close(it->first);
	}
	conns_.clear();
	for (size_t i = 0; i < listener_.size(); ++i)
		close(listener_[i].fd());
	Logger::server("Cleanup complete");
}
