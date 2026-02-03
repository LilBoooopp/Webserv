#include "Server.hpp"
#include <cstdio>
#include <fstream>
#include <iosfwd>
#include <iostream>

static void handleGet(Connection &c) {
  const std::string kStreamHeader = "X-Stream-File";
  if (!c.res.hasHeader(kStreamHeader))
    return;

  std::string file_path = c.res.getHeader(kStreamHeader);
  //	Try to open the file now, in non-streaming, blocking mode.
  //	We will only read it in small chunks later form handleWritable.
  int ffd = ::open(file_path.c_str(), O_RDONLY);
  if (ffd >= 0) {
    c.file_fd = ffd;
    c.streaming_file = true;

    //	Initialize remaining bytes from Content-Length
    std::string cl = c.res.getHeader("Content-Length");
    off_t remaining = 0;
    if (!cl.empty()) {
      std::istringstream iss(cl);
      iss >> remaining;
    }
    c.file_remaining = remaining;

    //	Remove the internal header so the client doesn't see it.
    c.res.eraseHeader(kStreamHeader);
  } else {
    //	If open fails, fall back to a 404
    c.res.setStatusFromCode(404);

    // ensure no streaming
    c.streaming_file = false;
    c.file_fd = -1;
    c.file_remaining = 0;
  }
}

static void handlePost(Connection &c) {
  const std::string &target_file =
      Router::resolvePath(c.cfg, c.loc, c.req.target);

  std::ifstream in(c.temp_filename.c_str(), std::ios::binary);
  if (!in.is_open()) {
    Logger::error("Could not open temp file for processing: %s",
                  c.temp_filename.c_str());
    c.res.setStatusFromCode(500);
    return;
  }

  // Open file for writing (will create or overwrite)
  std::ofstream out(target_file.c_str(), std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    Logger::error("Could not open target file for writing: %s",
                  target_file.c_str());
    in.close();
    c.res.setStatusFromCode(500);
    return;
  }

  std::streampos start_pos = 0;
  std::streampos end_pos = 0;

  in.seekg(0, std::ios::end);
  std::streampos file_size = in.tellg();
  in.seekg(0, std::ios::beg);

  end_pos = file_size;

  std::string contentType = c.req.getHeader("Content-Type");
  bool is_multipart =
      contentType.find("multipart/form-data") != std::string::npos;
  if (is_multipart) {
    size_t pos = contentType.find("boundary=");
    if (pos != std::string::npos) {
      std::string boundary = contentType.substr(pos + 9);

      const size_t HEADER_SCAN_SIZE = 4096;
      char buf[HEADER_SCAN_SIZE];
      in.read(buf, HEADER_SCAN_SIZE);
      std::string header_chunk(buf, in.gcount());

      size_t header_end = header_chunk.find("\r\n\r\n");
      if (header_end != std::string::npos)
        start_pos = header_end + 4;

      in.seekg(start_pos);

      size_t footer_len = 2 + 2 + boundary.length() + 2;
      if (static_cast<size_t>(file_size) >
          (static_cast<size_t>(start_pos) + footer_len)) {
        end_pos = static_cast<size_t>(file_size) - footer_len;
      }
    }
  }
  const size_t BUF_SIZE = 65536;
  char buffer[BUF_SIZE];

  size_t total_to_write =
      static_cast<size_t>(end_pos) - static_cast<size_t>(start_pos);
  size_t current_written = 0;

  while (current_written < total_to_write && in.good() && out.good()) {
    size_t chunk = BUF_SIZE;
    if (total_to_write - current_written < chunk)
      chunk = total_to_write - current_written;

    in.read(buffer, chunk);
    std::streamsize bytes_read = in.gcount();

    if (bytes_read > 0) {
      out.write(buffer, bytes_read);
      current_written += bytes_read;
    } else {
      break;
    }
  }

  in.close();
  out.close();

  if (!c.temp_filename.empty()) {
    std::remove(c.temp_filename.c_str());
    c.temp_filename.clear();
  }
  c.res.setStatusFromCode(201);
}

static void handleDelete(Connection &c) {
  if (!c.loc) {
    c.res.setStatusFromCode(403);
    c.res.setHeader("Allow", "GET, HEAD");
    return;
  }
  if (!Router::checkAllowedMethod(c, *c.loc))
    return;
  const std::string &target_file =
      Router::resolvePath(c.cfg, c.loc, c.req.target);
  Logger::response("%srouted \'%s%s%s\' -> %s%s%s", GREY, URLCLR,
                   c.req.target.c_str(), TS, URLCLR, target_file.c_str(), TS);
  if (!file_exists(target_file))
    c.res.setStatusFromCode(404);
  else if (remove(target_file.c_str()) != 0)
    c.res.setStatusFromCode(500);
  else {
    Logger::response("file at path \'%s%s%s\' deleted.", URLCLR,
                     target_file.c_str(), TS);
    c.res.setStatusFromCode(204);
  }
}

// Build and serialize a response once the request/body are ready
void Server::prepareResponse(int fd, Connection &c) {
  if (c.res.getStatus() >= 400) {
    Router::finalizeResponse(c);
    enableWrite(fd);
    return;
  }

  c.req.log();
  c.res.setVersion(c.req.version);

  if (is_cgi(c.req.target, c.cfg)) {
    if (!cgiHandler_.runCgi(c, fd)) {
      Router::finalizeResponse(c);
      enableWrite(fd);
    } else {
      c.state = WAITING_CGI;
    }
    return;
  }
  if (!Router::route(c) && (c.req.method == "GET" || c.req.method == "HEAD"))
    Router::handle(c);
  if (c.req.method == "GET")
    handleGet(c);
  else if (c.req.method == "POST")
    handlePost(c);
  else if (c.req.method == "DELETE")
    handleDelete(c);
  else if (c.req.method != "HEAD")
    c.res.setStatusFromCode(405);

  // Set Connection header based on client request
  std::string connection_header = c.req.getHeader("connection");
  if (connection_header == "close" || connection_header == "Close") {
    c.res.setHeader("Connection", "close");
  } else {
    c.res.setHeader("Connection", "keep-alive");
  }

  Router::finalizeResponse(c);
  enableWrite(fd);
}
