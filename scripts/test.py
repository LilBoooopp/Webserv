#!/usr/bin/env python3
import socket
import time
import sys
import threading
import random
import os
import re

# ==============================================================================
# CONFIGURATION
# ==============================================================================
HOST = "127.0.0.1"
PORT = 8000
SERVER_BIN = "./webserv"
CONFIG_FILE = "default.conf"

# Paths must match your webserv config!
PATH_ROOT = "/"
PATH_NOT_FOUND = "/wubba_lubba_dub_dub"
PATH_UPLOAD = "/uploads/postBytes/"  # Must accept POST and DELETE
PATH_CGI = "/cgi-bin/test.php" # Optional: set to None to skip CGI tests

# Timeouts
CONNECTION_TIMEOUT = 2.0  # Seconds to wait for connection
READ_TIMEOUT = 2.0        # Max time to wait for data if length unknown

# ==============================================================================
# UTILS & COLORS
# ==============================================================================
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

def print_pass(name, details=""):
    print(f"{Colors.OKGREEN}[PASS]{Colors.ENDC} {name} {details}")

def print_fail(name, details=""):
    print(f"{Colors.FAIL}[FAIL]{Colors.ENDC} {name}")
    print(f"{Colors.FAIL}       Error: {details}{Colors.ENDC}")

# ==============================================================================
# SMART HTTP READER
# ==============================================================================
def read_http_response(s):
    """
    Reads from socket until the full HTTP response is received.
    Parses Content-Length or Chunked encoding to avoid waiting for timeouts.
    """
    s.settimeout(READ_TIMEOUT)
    data = b""
    header_part = b""
    body_part = b""
    content_length = -1
    is_chunked = False
    headers_parsed = False

    while True:
        try:
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
            
            # 1. Parse Headers if we haven't yet
            if not headers_parsed:
                if b"\r\n\r\n" in data:
                    headers_end = data.find(b"\r\n\r\n") + 4
                    header_part = data[:headers_end]
                    body_part = data[headers_end:]
                    headers_parsed = True
                    
                    # Check for Content-Length
                    cl_match = re.search(rb"Content-Length:\s*(\d+)", header_part, re.IGNORECASE)
                    if cl_match:
                        content_length = int(cl_match.group(1))
                    
                    # Check for Chunked
                    if b"Transfer-Encoding: chunked" in header_part:
                        is_chunked = True
            
            # 2. Check if we are done reading based on parsed headers
            if headers_parsed:
                # Case A: Content-Length known
                if content_length != -1:
                    if len(body_part) >= content_length:
                        # We have the full body (and maybe more, though unlikely in simple tests)
                        break
                
                # Case B: Chunked Encoding
                elif is_chunked:
                    # Look for the end of the zero chunk: 0\r\n\r\n
                    if body_part.endswith(b"0\r\n\r\n"):
                        break
                
                # Case C: No Body (e.g., 204, 304) or unknown length (Connection: close)
                # If we can't determine length, we rely on the socket closing or timeout,
                # but valid HTTP/1.1 usually gives us CL or Chunked.
                
        except socket.timeout:
            break
            
    return data

# ==============================================================================
# TEST FUNCTIONS
# ==============================================================================
def send_request(name, request_data, expect_status=200, expect_body=None, check_pipelining=False):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(CONNECTION_TIMEOUT)
            s.connect((HOST, PORT))
            
            if isinstance(request_data, str):
                request_data = request_data.encode()
            
            s.sendall(request_data)
            response = read_http_response(s)
            
            if not response:
                print_fail(name, "No response received")
                return False

            # Decode headers for checking (latin-1 is safe for headers)
            try:
                head_str = response.split(b"\r\n\r\n")[0].decode('latin-1')
            except:
                head_str = str(response)

            # Check Status Code
            status_line = head_str.split('\r\n')[0]
            if str(expect_status) not in status_line:
                print_fail(name, f"Expected {expect_status}, got '{status_line}'")
                return False

            # Check Pipelining (Special Case)
            if check_pipelining:
                # Count how many times "HTTP/1.1" appears in the response
                # (This assumes the server sends responses back-to-back)
                count = response.count(b"HTTP/1.1")
                if count < 2:
                    print_fail(name, f"Pipelining failed. Expected 2 responses, got {count}")
                    return False

            # Check Body Content (if requested)
            if expect_body:
                if isinstance(expect_body, str):
                    expect_body = expect_body.encode()
                if expect_body not in response:
                    print_fail(name, "Body content mismatch")
                    return False

            print_pass(name, f"({status_line})")
            return True

    except ConnectionRefusedError:
        print_fail(name, "Connection refused. Is the server running?")
        return False
    except Exception as e:
        print_fail(name, str(e))
        return False

# ==============================================================================
# SUITES
# ==============================================================================

def run_basic_tests():
    print(f"\n{Colors.BOLD}=== BASIC REQUESTS ==={Colors.ENDC}")
    
    # 1. GET Root
    req = f"GET {PATH_ROOT} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
    send_request("GET Root", req, expect_status=200)

    # 2. GET 404
    req = f"GET {PATH_NOT_FOUND} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
    send_request("GET Non-existent", req, expect_status=404)

    # 3. DELETE (Not Allowed on Root usually)
    req = f"DELETE {PATH_ROOT} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
    # This might be 405 (Method Not Allowed) or 403 (Forbidden) depending on your config
    send_request("DELETE Root (Expect Fail)", req, expect_status=405)

def run_upload_tests():
    print(f"\n{Colors.BOLD}=== UPLOAD & POST ==={Colors.ENDC}")
    
    filename = f"test_{random.randint(1000, 9999)}.txt"
    file_content = "This is a test file content for webserv."
    
    # 1. POST File
    # Note: This is a raw POST. If your server expects Multipart, this might fail unless 
    # you configured a raw upload endpoint. Assuming raw POST for simplicity:
    req = (
        f"POST {PATH_UPLOAD}/{filename} HTTP/1.1\r\n"
        f"Host: {HOST}\r\n"
        f"Content-Length: {len(file_content)}\r\n"
        f"Content-Type: text/plain\r\n\r\n"
        f"{file_content}"
    )
    
    if send_request("POST Upload File", req, expect_status=201):
        
        # 2. GET the uploaded file
        req_get = f"GET {PATH_UPLOAD}/{filename} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
        send_request("GET Uploaded File", req_get, expect_status=200, expect_body=file_content)
        
        # 3. DELETE the file
        req_del = f"DELETE {PATH_UPLOAD}/{filename} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
        send_request("DELETE Uploaded File", req_del, expect_status=204) # Or 200/202

        # 4. Verify Deletion
        send_request("GET Deleted File", req_get, expect_status=404)

def run_advanced_tests():
    print(f"\n{Colors.BOLD}=== ADVANCED HTTP ==={Colors.ENDC}")

    # 1. Chunked Encoding Request
    # We send the body in two chunks: "Hello " and "World!"
    chunk1 = b"6\r\nHello \r\n"
    chunk2 = b"6\r\nWorld!\r\n"
    end = b"0\r\n\r\n"
    
    filename = "chunked_test.txt"
    
    headers = (
        f"POST {PATH_UPLOAD}/{filename} HTTP/1.1\r\n"
        f"Host: {HOST}\r\n"
        f"Transfer-Encoding: chunked\r\n\r\n"
    ).encode()
    
    full_req = headers + chunk1 + chunk2 + end
    
    # We expect the server to reconstruct "Hello World!"
    if send_request("Chunked POST", full_req, expect_status=201):
         # Verify content
         req_get = f"GET {PATH_UPLOAD}/{filename} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
         send_request("Verify Chunked Content", req_get, expect_status=200, expect_body="Hello World!")
         
         # Cleanup
         req_del = f"DELETE {PATH_UPLOAD}/{filename} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
         send_request("Cleanup Chunked", req_del, expect_status=204) # Or 200

    # 2. Pipelining
    # Send two GET requests in a single write operation
    print(f"{Colors.OKBLUE}Testing Pipelining (2 requests in 1 socket send)...{Colors.ENDC}")
    pipe_req = (
        f"GET {PATH_ROOT} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
        f"GET {PATH_ROOT} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
    )
    send_request("Pipelining", pipe_req, expect_status=200, check_pipelining=True)

def run_stress_test(count=50):
    print(f"\n{Colors.BOLD}=== STRESS TEST ({count} reqs) ==={Colors.ENDC}")
    
    def worker(t_id, results):
        req = f"GET {PATH_ROOT} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                s.sendall(req.encode())
                resp = s.recv(1024)
                if b"200 OK" in resp:
                    results.append(True)
                else:
                    results.append(False)
        except:
            results.append(False)

    threads = []
    results = []
    
    print("Launching threads...")
    start = time.time()
    
    for i in range(count):
        t = threading.Thread(target=worker, args=(i, results))
        t.start()
        threads.append(t)
        
    for t in threads:
        t.join()
        
    end = time.time()
    success_count = results.count(True)
    duration = end - start
    
    if success_count == count:
        print(f"{Colors.OKGREEN}[PASS]{Colors.ENDC} {success_count}/{count} requests succeeded in {duration:.2f}s")
    else:
        print(f"{Colors.FAIL}[FAIL]{Colors.ENDC} Only {success_count}/{count} requests succeeded")

# ==============================================================================
# MAIN
# ==============================================================================
if __name__ == "__main__":
    print(f"{Colors.BOLD}Starting Webserv Tests on {HOST}:{PORT}{Colors.ENDC}")
    
    # Ensure server is running (User must start it manually or use a wrapper)
    # Simple connectivity check
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(1)
            s.connect((HOST, PORT))
    except:
        print(f"{Colors.FAIL}Error: Could not connect to {HOST}:{PORT}. Is webserv running?{Colors.ENDC}")
        sys.exit(1)

    run_basic_tests()
    run_upload_tests()
    run_advanced_tests()
    run_stress_test(100)
    
    print(f"\n{Colors.BOLD}Test Suite Completed.{Colors.ENDC}")
