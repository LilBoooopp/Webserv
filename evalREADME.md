# Webserv

*This project has been created as part of the 42 curriculum by akabbaj, cbopp, gvalente.*

## Description

Webserv is a lightweight, non-blocking HTTP/1.1 server written in C++98, designed to replicate the core functionality of NGINX. The project focuses on understanding HTTP internals, socket programming, and event-driven architecture.

It utilizes a single-threaded reactor pattern with `epoll` (Linux) or `kqueue` (macOS) to handle multiple concurrent client connections efficiently. The server supports a comprehensive configuration system, CGI execution (PHP, Python), and standard HTTP methods, making it a fully functional web server for static and dynamic content.

### Key Architecture
* **Event-Driven:** Non-blocking I/O using a single `poll()` equivalent (epoll/kqueue) to manage thousands of connections.
* **Zero-Copy Streaming:** Efficient file serving that streams content from disk to socket.
* **State Machine:** Per-connection state tracking (Reading Headers → Reading Body → Routing → Writing Response).

## Instructions

### Compilation
The project includes a Makefile that compiles the source files without unnecessary relinking.

```bash
make            # Standard build (strict flags: -Wall -Wextra -Werror -std=c++98)
make debug      # Build with AddressSanitizer for memory debugging
make clean      # Remove object files
make fclean     # Remove objects and binary
make re         # Clean rebuild
```

### Execution

Run the server by providing a configuration file. If no file is provided, it defaults to default.conf.
```bash
./webserv [/path/to/configuration_file]
```

### Configuration

The server uses an NGINX-inspired configuration syntax. Below is a sample configuration:

```nginx
server {
    listen 8000;
    server_name localhost;
    root www_giulio/;
    index index.html;
    client_max_body_size 1M;
    
    error_page 404 /errors/404.html;
    
    location /cgi-bin {
        cgi .php /usr/bin/php-cgi;
        cgi .py /usr/bin/python3;
        cgi_timeout 5000;
    }
    
    location /uploads {
        allowed_methods GET POST DELETE;
        client_max_body_size 50M;
    }
}
```

Invalid root : segfault when url accessed 
Duplicate methods handled how?
Duplicate CGIs

The following values can be set in the global scope:
 * Max body size (default: 1024 * 1024)
 * Timeout (default: 5000ms)
 * Root location
 * Index files
If the values are not set where necessary in server these values will be inherited.
Similarly if these values are not set in the location block, they will inherit from the corresponding server block.

CGI timeouts will inherit from server timeouts if not set.
CGI max output will inherit from location max body size.

If no allowed methods are set, GET will be set by default.

## Features List
### Mandatory Part

 * HTTP Protocol: Full support for GET, POST, DELETE methods and HTTP/1.1 headers.

  * I/O Multiplexing: epoll (Linux) and kqueue (macOS) integration.

  * CGI Support: Execution of PHP and Python scripts via child processes with environment setup (REQUEST_METHOD, QUERY_STRING, etc.).

  * Static Serving: Directory listing, static files, and customizable index files.

  * Uploads: Multipart form data handling and file storage.

  * Error Handling: Custom error pages (404, 403, 500, etc.) and graceful failure handling.

Resilience: Signal handling (SIGINT) for graceful shutdown and resource cleanup.

### Bonus Part

 * Cookies & Sessions: Session management implementation using HTTP cookies.

 * Multiple CGI: Support for multiple CGI interpreters (PHP and Python) configured via file extensions.

## Resources
### References

 * RFC 7230-7235 (HTTP/1.1 Specification)

 * RFC 3875 (The Common Gateway Interface)

 * Beej's Guide to Network Programming
 
 * Man pages: epoll(7), kqueue(2), socket(2)

 * NGINX Documentation

### AI Usage
In compliance with the AI instructions for this project, AI tools were used for the following tasks:

 * **Concept Explanation:** AI was used to explain unclear implementations of features, notably cookies/session ids.
 
 * **Debugging:** Used to understand obscure C++98 compilation errors.

 * **Testing:** Generated ideas for edge-case tests (e.g., malformed headers, large chunked requests) to ensure server stability.

 * **Makefile creation:** Adapted existing Makefile to conform to new subject regulations.

*No code was copy-pasted directly into the final codebase without review, testing, and understanding.*
