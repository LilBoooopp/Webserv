#include "../config/Config.hpp"
#include "../utils/Utils.hpp"
#include "Router.hpp"
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// Threshold above which we stream instead of loading into memory
static const off_t STREAM_THRESHOLD = 256 * 1024; // 256 KB

/**
 * @brief Convert a file size (off_t) to std::string
 */
static std::string size_to_str(off_t n) {
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

/**
 * @brief Read entire file into 'out'.
 * @return true on sucess, false on any open/read failure.
 */
static bool read_file_small(const std::string &path, std::string &out) {
	int fd = ::open(path.c_str(), O_RDONLY);
	if (fd < 0)
		return (false);

	std::vector<char> buf(64 * 1024);
	out.clear();

	bool got_any = false;

	for (;;) {
		ssize_t r = ::read(fd, &buf[0], buf.size());
		if (r > 0) {
			got_any = true;
			out.append(&buf[0], r);
		} else
			break;
	}
	::close(fd);
	return (got_any);
}

std::string buildDirListing(const std::string &root, const std::string &uri) {
	std::ostringstream html;

	const std::string path = root + uri;
	std::string prev = uri;
	if (prev.size() > 1 && prev.back() == '/')
		prev.pop_back();
	size_t last = prev.find_last_of('/');
	if (last != std::string::npos)
		prev = prev.substr(0, last + 1);
	else
		prev = "/"; // fallback to root
	html << "<html><head><title>Index of " << uri << "</title></head>\n";
	html << "<body><h1>Index of " << uri << "</h1>\n";
	html << "<table border=\"0\" cellpadding=\"10\">\n";
	html << "<tr><th>Name</th><th>Type</th><th>Modified</th><th>Size</th></tr>\n";
	html << "<tr><td><a href=\"" << prev << "\">../</a></td><td></td><td></td><td></td></tr>\n";

	DIR *dir = opendir(path.c_str());
	if (dir) {
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;
			if (name == "." || name == "..")
				continue;
			struct stat st;
			stat((path + "/" + name).c_str(), &st);
			time_t mtime = st.st_mtime;
			struct tm *timeinfo = localtime(&mtime);
			char buffer[80];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
			std::string mtime_str(buffer);
			std::string fileType = (is_dir(st)	      ? "dir"
						: S_ISREG(st.st_mode) ? "file"
								      : "other");
			std::string showName = name + (fileType == "dir" ? "/" : "");
			std::string href = uri;
			if (!href.empty() && href.back() != '/')
				href += '/';
			href += name;
			std::string sizeStr =
			    (fileType == "dir") ? "-" : size_to_str(st.st_size / 1000) + "K";
			html << "<tr><td><a href=\"" << href << "\">" << showName << "</a></td><td>"
			     << fileType << "</td><td>" << mtime_str << "</td><td>" << sizeStr
			     << "</td></tr>\n";
		}
		closedir(dir);
	}
	html << "</table></body></html>\n";
	return html.str();
}

static void respondWithDirListing(Connection &c) {
	std::string dirHtml = buildDirListing(c.cfg.root, c.req.target);
	c.res.setContentType("text/html");
	if (c.req.method == "HEAD") {
		c.res.setStatus(200, "OK");
		c.res.setHeader("Content-Length", size_to_str(static_cast<off_t>(dirHtml.size())));
	} else {
		c.res.setStatus(200, "OK");
		c.res.setBody(dirHtml);
	}
}

void Router::handle(Connection &c) {
	// We assume: method validated (ONLY GET for now)
	const std::string &method = c.req.method;
	const bool is_head = (method == "HEAD");

	const std::vector<std::string> *index_files = &c.cfg.files;

	if (c.loc && c.loc->has_index)
		index_files = &c.loc->index_files;

	// Map the request target to a safe filesystem path under cfg.root
	std::string path = Router::resolvePath(c.cfg, c.loc, c.req.target);

	Logger::router("%srouted %s%s%s -> %s%s", GREY, URLCLR, c.req.target.c_str(), GREY,
		       URLCLR, path.c_str());

	struct stat st;
	if (::stat(path.c_str(), &st) == 0) {
		// Is a Directory
		if (is_dir(st)) {
			// try index file: root/target[/] + cfg.index
			if (path.size() == 0 || path[path.size() - 1] != '/')
				path += '/';

			if (!index_files->empty()) {
				std::string idx = path + index_files->at(0);

				if (file_exists(idx) && is_reg(idx)) {
					c.res.setContentType(mime_from_path(idx));
					if (is_head) {
						c.res.setStatus(200, "OK");
						c.res.setHeader("Content_Length",
								size_to_str(st.st_size));
						return;
					}

					if (st.st_size <= STREAM_THRESHOLD) {
						std::string body;
						if (read_file_small(idx, body)) {
							c.res.setStatus(200, "OK");
							c.res.setBody(body);
							return;
						}
					} else {
						c.res.setStatus(200, "OK");
						c.res.setHeader("Content_Length",
								size_to_str(st.st_size));
						c.res.setHeader("X-Stream-File", idx);
						return;
					}
				} else {
					if (c.loc && c.loc->autoindex) {
						Logger::response(
						    "index \'%s\' missing -> dir listing",
						    idx.c_str());
						respondWithDirListing(c);
						return;
					}
					// Index file configured but doesn't exist → 404
					Logger::response("index \'%s\' missing -> 404",
							 idx.c_str());
					c.res.setStatusFromCode(404);
					c.res.ensureDefaultBodyIfEmpty();
					return;
				}
			} else if (c.loc && c.loc->autoindex) {
				Logger::response("location at path %s doesn't have an index "
						 "-> dir listing",
						 path.c_str());
				respondWithDirListing(c);
				return;
			}
			// Directory exists but no index configured and no autoindex → 403
			Logger::response("directory %s has no index file "
					 "configured -> sending 403",
					 path.c_str());
			c.res.setStatusFromCode(403);
			c.res.ensureDefaultBodyIfEmpty();
			return;
		}
		// Is a Regular File
		else if (is_reg(st)) {
			c.res.setContentType(mime_from_path(path));
			// For HEAD, we don't read the file, we just set headers
			if (is_head) {
				c.res.setStatus(200, "OK");
				// Use stat's size for Content_Length
				c.res.setHeader("Content-Length", size_to_str(st.st_size));
				return;
			}

			// Decide between small read vs streaming
			if (st.st_size <= STREAM_THRESHOLD) {
				std::string body;
				if (read_file_small(path, body)) {
					c.res.setStatus(200, "OK");
					c.res.setBody(body);
					// For GET, HttpResponse::serialize() can
					// establish Content-Length from body.size()
					return;
				}
			} else {
				// Large file: mark for streaming
				c.res.setStatus(200, "OK");
				c.res.setHeader("Content-Length", size_to_str(st.st_size));
				// Internal hint for Server: path to stream
				c.res.setHeader("X-Stream-File", path);
				return;
			}
		}
	}
	c.res.setStatusFromCode(404);
	c.res.ensureDefaultBodyIfEmpty();
}
