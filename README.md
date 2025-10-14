# MiniWebserv

## Project Overview
MiniWebserv is a lightweight HTTP server written in C++98, designed to help you understand the inner workings of HTTP and web servers. It is compatible with standard web browsers and can serve static websites, handle file uploads, and process basic HTTP methods (GET, POST, DELETE). The server is non-blocking and uses epoll for efficient I/O multiplexing.

## Features Implemented

### 1. Core Server Loop
- Uses epoll for non-blocking I/O and event-driven architecture (`core/EpollReactor.cpp`).
- Handles multiple client connections efficiently in a single-threaded loop.
- Listens on configurable IP and port (`server/Listener.cpp`, `server/Server.cpp`).

### 2. HTTP Request Parsing
- Parses HTTP/1.1 requests, including headers and request line (`http/HttpParser.cpp`).
- Validates requests and responds with appropriate status codes for errors.

### 3. Static File Serving
- Serves static files and directories from a configurable root (`server/StaticHandler.cpp`).
- Supports index files for directories and MIME type detection (`utils/Mime.hpp`).
- Returns 404 for missing files and directories.

### 4. Chunked Transfer Decoding
- Decodes chunked transfer encoding for incoming requests (`http/ChunkedDecoder.cpp`).
- Handles chunked uploads and trailers.

### 5. Basic POST Handling
- Accepts POST requests and echoes received data (placeholder for file upload logic).

### 6. Logging
- Provides logging at various levels (error, warn, info, debug) (`utils/Logger.cpp`).

### 7. Configuration
- Reads configuration for server settings (root, index, max body size, etc.) (`config/Config.hpp`).

## How It Works
- The server starts by parsing the configuration and initializing the listener socket.
- Incoming connections are accepted and set to non-blocking mode.
- Each client connection is managed via epoll events for read/write.
- Requests are parsed, validated, and routed to appropriate handlers (static files, POST, etc.).
- Responses are serialized and sent back to the client, then the connection is closed.

## Usage
```
make
./webserv [configuration file]
```
- The configuration file specifies server ports, root directory, index file, error pages, and route rules.

## TODO List

### 1. Configuration File Parsing
- Implement full-featured config file parser (NGINX-like syntax).
- Support multiple servers, ports, custom error pages, max body size, routes, redirects, directory listing, CGI, upload paths.

### 2. HTTP Methods
- Check how far GET request needs to go, what files to return, etc.
- Implement DELETE method logic.
- Extend POST to support file uploads and saving to disk.

### 3. CGI Support
- Add CGI execution for file extensions (e.g., .php, .py).
- Pass correct environment and arguments to CGI scripts.
- Handle chunked requests and CGI output correctly.

### 4. Error Handling
- Add default and custom error pages for all HTTP status codes.
- Improve resilience and stress testing.

### 5. Directory Listing
- Implement directory listing for routes where enabled.

### 6. Advanced Routing
- Add support for HTTP redirects, accepted methods per route, and route-specific configs.

### 7. Bonus Features
- Cookie and session management.
- Multiple CGI support.

## Integration Plan
- The configuration parser will feed settings into the existing `ServerConfig` structure, affecting how listeners, handlers, and routes are set up.
- CGI and upload logic will extend the POST/GET handling in `Server.cpp` and `StaticHandler.cpp`.
- Error handling and directory listing will be integrated into the static handler and response logic.
- Bonus features will build on the request/response and connection management already in place.

