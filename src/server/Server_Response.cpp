#include "Server.hpp"
#include <cstdio>
#include <fstream>
#include <iosfwd>
#include <iostream>
// #include <pstl/glue_algorithm_defs.h>

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

// static void handlePost(Connection &c) {
//   // 1. Resolve final path
//   std::string target_file = Router::resolvePath(c.cfg, c.loc, c.req.target);
//
//   // 2. Open the temporary file for reading
//   // Note: C++98 fstream constructor requires const char*, not std::string
//   std::ifstream in(c.temp_filename.c_str(), std::ios::binary);
//   if (!in.is_open()) {
//     Logger::error("Could not open temp file for processing: %s",
//                   c.temp_filename.c_str());
//     c.res.setStatusFromCode(500);
//     return;
//   }
//
//   // 3. Open the target file for writing
//   std::ofstream out(target_file.c_str(), std::ios::binary | std::ios::trunc);
//   if (!out.is_open()) {
//     Logger::error("Could not open target file for writing: %s",
//                   target_file.c_str());
//     in.close();
//     c.res.setStatusFromCode(500);
//     return;
//   }
//
//   // 4. Calculate Copy Range (Start & End)
//   std::streampos start_pos = 0;
//
//   // Get total file size
//   in.seekg(0, std::ios::end);
//   std::streampos file_size = in.tellg();
//   std::streampos end_pos = file_size;
//   in.seekg(0, std::ios::beg);
//
//   std::string contentType = c.req.getHeader("Content-Type");
//   bool is_multipart =
//       (contentType.find("multipart/form-data") != std::string::npos);
//
//   if (is_multipart) {
//     size_t pos = contentType.find("boundary=");
//     if (pos != std::string::npos) {
//       std::string boundary = contentType.substr(pos + 9);
//
//       // --- A. FIND DATA START (Skip Headers) ---
//       // Read the first 4KB to find the double CRLF
//       const size_t SCAN_SIZE = 4096;
//       char buf[SCAN_SIZE];
//
//       in.read(buf, SCAN_SIZE);
//       size_t read_bytes = static_cast<size_t>(in.gcount());
//       std::string head_chunk(buf, read_bytes);
//
//       size_t header_end = head_chunk.find("\r\n\r\n");
//       if (header_end != std::string::npos) {
//         // The file data starts 4 bytes after the \r\n\r\n
//         start_pos = static_cast<std::streampos>(header_end + 4);
//       }
//
//       // --- B. FIND DATA END (Remove Footer) ---
//       // The footer looks like: \r\n--boundary--
//       // We scan the end of the file to find where the boundary starts.
//
//       // We need a scan window large enough to hold \r\n--boundary--\r\n
//       size_t footer_scan_size = boundary.length() + 100;
//       if (footer_scan_size > static_cast<size_t>(file_size))
//         footer_scan_size = static_cast<size_t>(file_size);
//
//       in.seekg(-static_cast<std::streamoff>(footer_scan_size),
//       std::ios::end);
//
//       // Use a vector for the buffer to be safe with C++98 memory
//       std::vector<char> footer_buf(footer_scan_size);
//       in.read(&footer_buf[0], footer_scan_size);
//
//       // Convert to string for easy searching
//       std::string tail_chunk(&footer_buf[0], in.gcount());
//
//       // Construct the string we are looking for: "--boundary--"
//       std::string boundary_str = "--" + boundary + "--";
//
//       // Find the boundary in the tail
//       size_t boundary_loc = tail_chunk.rfind(boundary_str);
//       if (boundary_loc != std::string::npos) {
//         // We found the boundary. The data usually ends at the \r\n BEFORE
//         it.
//         // Check for the \r\n immediately preceding the boundary
//         if (boundary_loc >= 2 &&
//             tail_chunk.substr(boundary_loc - 2, 2) == "\r\n") {
//           boundary_loc -= 2;
//         }
//
//         // Calculate absolute position of the cut-off
//         std::streampos tail_start_offset =
//             file_size - static_cast<std::streamoff>(footer_scan_size);
//         end_pos = tail_start_offset +
//         static_cast<std::streamoff>(boundary_loc);
//       }
//     }
//   }
//
//   // 5. Perform the Streaming Copy
//   // Reset reading pointer to the calculated start
//   in.clear(); // Clear any eof flags from previous seeks
//   in.seekg(start_pos, std::ios::beg);
//
//   const size_t CHUNK_SIZE = 65536; // 64KB Copy Buffer
//   char buffer[CHUNK_SIZE];
//
//   // We use a manual loop to ensure we don't cross 'end_pos'
//   size_t total_to_copy = static_cast<size_t>(end_pos - start_pos);
//   size_t copied = 0;
//
//   while (copied < total_to_copy && in.good() && out.good()) {
//     size_t to_read = CHUNK_SIZE;
//     if ((total_to_copy - copied) < to_read)
//       to_read = total_to_copy - copied;
//
//     in.read(buffer, to_read);
//     std::streamsize r = in.gcount();
//     if (r > 0) {
//       out.write(buffer, r);
//       copied += static_cast<size_t>(r);
//     } else {
//       break;
//     }
//   }
//
//   // 6. Cleanup
//   in.close();
//   out.close();
//
//   // Delete the temporary file
//   if (!c.temp_filename.empty()) {
//     std::remove(c.temp_filename.c_str());
//     c.temp_filename = "";
//   }
// }

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
  else
    Logger::response("file at path \'%s%s%s\' deleted.", URLCLR,
                     target_file.c_str(), TS);
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
  Router::finalizeResponse(c);
  enableWrite(fd);
}
