# Webserv

A lightweight, non-blocking HTTP/1.1 server written in C++98 with event-driven architecture using epoll. Designed as a learning project to understand HTTP internals and modern web server patterns. Features NGINX-like configuration, CGI support (PHP, Python), and full HTTP request/response handling.

---

## Features Implemented ✅

### Core Server Architecture
- **Non-blocking Event Loop**: Single-threaded epoll-based reactor pattern handles multiple concurrent connections
- **Multi-port Support**: NGINX-like configuration with multiple server blocks listening on different ports
- **Platform Support**: Linux (epoll) and macOS (kqueue via compatibility layer)
- **Signal Handling**: Graceful shutdown on SIGINT with full resource cleanup

### HTTP Protocol
- **Request Parsing**: Full HTTP/1.1 request line, headers, and body parsing
- **Chunked Transfer Encoding**: Full support for Transfer-Encoding: chunked (request and response)
- **Content-Length**: Proper request body size validation and limiting
- **HTTP Methods**: GET, POST, PUT, DELETE, HEAD
- **Response Generation**: Complete HTTP/1.1 responses with status codes, headers, and body
- **MIME Type Detection**: Automatic Content-Type detection for static files

### Static Content Serving
- **File Serving**: Stream static files with proper headers and range support
- **Directory Listing**: Configurable auto-index with HTML directory browsing
- **Index Files**: Support for configurable index file names (e.g., index.html)
- **Error Pages**: Custom error pages for HTTP status codes (400, 401, 403, 404, 405, 413, 500, 501, 502, 503)

### Routing & Configuration
- **NGINX-like Config Parser**: Full-featured `server`, `location`, `listen`, `root`, `index` blocks
- **Location Matching**: Longest-prefix matching for request paths to location blocks
- **Method Control**: Per-location allowed methods configuration
- **HTTP Redirects**: 307/308 temporary/permanent redirects with configurable targets
- **Root & Index Configuration**: Per-location root and index file overrides

### CGI Execution
- **Multiple Interpreter Support**: PHP (php-cgi) and Python (python3) out-of-the-box
- **Async CGI Processing**: Non-blocking CGI execution via child processes and pipe monitoring
- **CGI Environment**: Full CGI environment setup (REQUEST_METHOD, QUERY_STRING, PATH_INFO, etc.)
- **Output Limits**: Configurable `cgi_max_output` to prevent resource exhaustion
- **Timeout Handling**: Configurable CGI execution timeouts with process cleanup

### File Upload Handling
- **Multipart Form Data**: Support for file uploads via POST requests
- **Upload Destinations**: Configurable upload paths per location
- **Size Limits**: Per-location `client_max_body_size` enforcement
- **Python CGI Upload Handler**: Automatic filename collision detection and handling

### Request Validation & Security
- **Max Body Size Enforcement**: Prevent oversized requests (413 Payload Too Large)
- **Method Validation**: Per-location allowed methods with 405 Method Not Allowed responses
- **Path Traversal Prevention**: Safe path joining prevents directory traversal attacks
- **Connection Timeouts**: Configurable idle connection timeout with cleanup

### Performance & Debugging
- **Logging System**: Colored, leveled logging throughout the codebase (DEBUG, INFO, WARN, ERROR)
- **Connection Tracking**: Per-connection state machine (READING_HEADERS, READING_BODY, READY_TO_RESPOND, WRITING_RESPONSE, etc.)
- **Debug Mode**: AddressSanitizer support (`make debug`)
- **Config Debug Output**: Detailed configuration parsing and validation logging

---

## TODO List 📋

Based on the official project specifications (webserv.txt), the following items are on the roadmap:

### Mandatory Requirements (Core) ✅
- ✅ Non-blocking I/O with single poll() equivalent (epoll/kqueue)
- ✅ Configuration file support
- ✅ HTTP GET, POST, DELETE methods
- ✅ Static file serving
- ✅ File upload support
- ✅ Default error pages
- ✅ Browser compatibility

### Bonus Features (Optional)
- ✅ **Cookies and Session Management**: Track user sessions with HTTP cookies
- ✅ **Multiple CGI Types**: Support for additional scripting languages beyond PHP and Python

### Known Implementation Gaps (Not in Official Spec)
- **CGI Streaming**: CGI output fully buffered before sending (not streamed)
- **SSL/TLS**: No HTTPS support
- **Content Encoding**: No gzip/deflate compression

---

## Building & Running

### Build
```bash
make              # Standard build with warnings-as-errors
make debug        # Build with AddressSanitizer for memory debugging
make clean        # Remove object files
make fclean       # Remove objects and binary
make re           # Clean rebuild
```

### Run
```bash
./webserv [config_file]     # Use config file (default: default.conf)
./webserv default.conf      # Explicit config path
```

### Configuration Example
```nginx
server {
    listen 127.0.0.1:8000;
    server_name webserv;
    
    root www_giulio/;
    index index.html;
    client_max_body_size 1M;
    
    error_page 404 /errors/404.html;
    
    location /cgi-bin {
        cgi .php /usr/bin/php-cgi;
        cgi .py /usr/bin/python3;
        cgi_timeout 5000;
        cgi_max_output 10M;
    }
    
    location /uploads {
        allowed_methods GET POST DELETE;
        client_max_body_size 50M;
    }
}
```

---

## Testing

### Manual Testing
```bash
# Start server
./webserv default.conf &

# Test GET request
curl http://127.0.0.1:8000/

# Test POST with file upload
curl -F "file=@/path/to/file" http://127.0.0.1:8000/uploads

# Test CGI
curl "http://127.0.0.1:8000/cgi-bin/script.php?param=value"

# Kill server
pkill webserv
```

---

## Project Structure

```
src/
├── main.cpp                    # Entry point
├── server/
│   ├── Server.cpp/hpp          # Main server loop & connection management
│   ├── Router.cpp/hpp          # Request routing & location matching
│   ├── Router_Handler.cpp      # Route handler implementations
│   ├── Server_Response.cpp     # Response preparation
│   └── Listener.cpp/hpp        # TCP listener socket management
├── core/
│   ├── EpollReactor.cpp/hpp    # Event multiplexing (epoll/kqueue)
│   └── EpollMac.hpp            # macOS kqueue compatibility
├── http/
│   ├── HttpParser.cpp/hpp      # HTTP request parsing
│   ├── HttpRequest.cpp/hpp     # Request object
│   ├── HttpResponse.cpp/hpp    # Response object
│   ├── Connection.cpp/hpp      # Per-connection state
│   └── ChunkedDecoder.cpp/hpp  # Transfer-Encoding: chunked
├── config/
│   ├── Config.cpp              # Config file parsing & validation
│   ├── Config.hpp              # Config data structures
│   ├── Config_Debug.cpp        # Debug output
│   ├── Config_Error.cpp        # Error handling
│   ├── Config_Helpers.cpp      # Helper functions
│   └── Config_Validation.cpp   # Validation logic
├── cgi/
│   ├── Cgi.hpp                 # CGI handler interface
│   ├── CgiData.hpp             # CGI process data
│   ├── Execute.cpp             # Process execution
│   ├── CgiTools.cpp            # Environment setup
│   └── Respond.cpp             # CGI response handling
└── utils/
    ├── Logger.cpp/hpp          # Logging system
    ├── Chrono.cpp/hpp          # Timing utilities
    ├── Colors.hpp              # ANSI color codes
    ├── Mime.hpp                # MIME type detection
    ├── Path.hpp                # Path utilities
    ├── File.hpp                # File I/O helpers
    └── Utils.hpp               # General utilities

www_giulio/                      # Example web root with content
├── index.html
├── cgi-bin/                     # CGI scripts (PHP, Python)
├── errors/                      # Error page templates
├── ressources/                  # Static content
└── uploads/                     # Upload destination

scripts/
├── tests.sh                     # Integration test suite
└── test.py                      # Python test utilities
```

---

## Architecture Highlights

### Event-Driven Design
The server uses a single-threaded event loop with epoll to handle thousands of concurrent connections. Each connection progresses through a state machine: reading headers → reading body → routing → writing response.

### Zero-Copy Streaming
Large files are streamed directly from disk to socket without buffering the entire file in memory, enabling efficient handling of large responses.

### NGINX-like Configuration
Configuration uses NGINX-familiar syntax with server blocks, location blocks, and per-location directives, making it intuitive for those familiar with production web servers.

### Async CGI Execution
CGI scripts execute in child processes; output is collected asynchronously via epoll, preventing blocking on slow scripts.

---

## Known Limitations

- **C++98 Only**: No C++11+ features; limits template usage and modern conveniences
- **Single-Threaded**: No thread pool or multi-process worker model
- **Memory Efficiency**: CGI output fully buffered before sending (not streamed)
- **No Clustering**: Cannot distribute load across multiple servers
- **No Caching**: Every request is processed fresh
- **Limited Logging**: Debug logging only; no structured access logging

---

## Future Improvements

1. **HTTP/2 Support**: Multiplexed streams and server push
2. **SSL/TLS**: HTTPS support via OpenSSL
4. **WebSocket**: Upgrade mechanism for real-time bidirectional communication
5. **Load Balancing**: Upstream and health check support
6. **Caching**: Response and asset caching strategies
7. **Rate Limiting**: Per-IP and per-path request throttling
8. **Authentication**: Built-in HTTP Basic/Digest auth
9. **Access Logging**: Combined/JSON format logging to files

---

## License

???
---

## References

- [HTTP/1.1 Specification (RFC 7230-7235)](https://tools.ietf.org/html/rfc7230)
- [CGI Specification (RFC 3875)](https://tools.ietf.org/html/rfc3875)
- [epoll Manual (Linux)](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [NGINX Documentation](https://nginx.org/)

