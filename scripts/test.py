#!/usr/bin/env python3
import subprocess
import socket
import time
import sys
import threading
import random
import os
from contextlib import closing

HOST = "127.0.0.1"
PORT = 8080

# Adjust this if your webserv needs a config file:
# e.g. ["./webserv", "config/webserv.conf"]
WEBSERV_CMD = ["../webserv", "default.conf"]

# How long to wait for the server to start responding
SERVER_START_TIMEOUT = 5.0
# Socket timeout for each test connection
SOCKET_TIMEOUT = 2.0
# Max bytes to read from response (to avoid dumping huge bodies)
MAX_READ_BYTES = 64 * 1024


class HttpTestResult:
    def __init__(self, name, ok, error=None, status_line=None, preview=None):
        self.name = name
        self.ok = ok
        self.error = error
        self.status_line = status_line
        self.preview = preview


def send_raw_request(raw_request):
    """
    Opens a TCP connection, sends raw HTTP request bytes, reads response.
    Returns raw response as bytes.
    """
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
        s.settimeout(SOCKET_TIMEOUT)
        s.connect((HOST, PORT))
        if isinstance(raw_request, bytes):
            s.sendall(raw_request)
        else:
            s.sendall(raw_request.encode("ascii"))
        chunks = []
        total = 0
        while total < MAX_READ_BYTES:
            try:
                data = s.recv(4096)
            except socket.timeout:
                break
            if not data:
                break
            chunks.append(data)
            total += len(data)
        return b"".join(chunks)


def parse_status_line(response_bytes):
    """
    Extracts the first line (status line) from an HTTP response.
    Returns it as a decoded string, or None if it can't be parsed.
    """
    try:
        text = response_bytes.decode("iso-8859-1", errors="replace")
    except Exception:
        return None
    end = text.find("\r\n")
    if end == -1:
        return None
    return text[:end]


def preview_body(response_bytes, limit=120):
    """
    Returns a small preview of the response body (first N bytes decoded).
    Good to see if server is roughly doing what we expect.
    """
    try:
        text = response_bytes.decode("utf-8", errors="replace")
    except Exception:
        text = repr(response_bytes)
    # Split headers/body
    sep = text.find("\r\n\r\n")
    if sep == -1:
        return text[:limit].replace("\n", "\\n")
    body = text[sep + 4 :]
    if len(body) > limit:
        return body[:limit].replace("\n", "\\n") + "...(truncated)"
    return body.replace("\n", "\\n")


def run_test(name, raw_request, expect_status_prefix=None):
    """
    Runs a single HTTP test:
      - Sends raw_request
      - Gets response
      - Optionally checks that status line starts with expect_status_prefix
    Returns HttpTestResult.
    """
    try:
        resp = send_raw_request(raw_request)
        if not resp:
            return HttpTestResult(name, ok=False, error="No response received")

        status_line = parse_status_line(resp)
        prev = preview_body(resp)

        if expect_status_prefix and status_line:
            ok = status_line.startswith(expect_status_prefix)
        else:
            ok = status_line is not None

        return HttpTestResult(name, ok=ok, status_line=status_line, preview=prev)

    except Exception as e:
        return HttpTestResult(name, ok=False, error=str(e))


def run_concurrent(name, raw_request, count, expect_status_prefix=None):
    results = []
    lock = threading.Lock()

    def worker(idx):
        res = run_test(f"{name} [#{idx}]", raw_request, expect_status_prefix)
        with lock:
            results.append(res)

    threads = []
    for i in range(count):
        t = threading.Thread(target=worker, args=(i,))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()

    ok_count = sum(1 for r in results if r.ok)
    status_line = f"{ok_count}/{count} succeeded"
    preview = None
    if ok_count != count:
        failures = [r.error or r.status_line for r in results if not r.ok][:3]
        preview = " | ".join(f for f in failures if f)
    return HttpTestResult(name, ok=(ok_count == count), status_line=status_line, preview=preview)


def build_upload_request(path, body_bytes):
    headers = [
        "POST " + path + " HTTP/1.1",
        "Host: localhost",
        f"Content-Length: {len(body_bytes)}",
        "Content-Type: application/octet-stream",
        "X-Filename: stress.bin",
        "",
        "",
    ]
    return "\r\n".join(headers).encode("ascii") + body_bytes


def build_chunked_request(path, chunks):
    # chunks: list of bytes
    lines = ["POST " + path + " HTTP/1.1", "Host: localhost", "Transfer-Encoding: chunked", "", ""]
    raw = "\r\n".join(lines).encode("ascii")
    for c in chunks:
        raw += f"{len(c):X}\r\n".encode("ascii") + c + b"\r\n"
    raw += b"0\r\n\r\n"
    return raw


def run_stress_suite():
    results = []

    # 100 concurrent GET /
    get_req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
    results.append(run_concurrent("Burst GET / x100", get_req, 100, expect_status_prefix="HTTP/1.1 "))

    # 50 concurrent pipelined double GET
    pipelined_req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
    results.append(run_concurrent("Burst pipelined x50", pipelined_req, 50, expect_status_prefix="HTTP/1.1 "))

    # 20 concurrent small uploads
    small_body = os.urandom(1024)
    small_upload = build_upload_request("/upload", small_body)
    results.append(run_concurrent("Small uploads x20", small_upload, 20, expect_status_prefix="HTTP/1.1 "))

    # 10 concurrent 64KB uploads
    big_body = os.urandom(64 * 1024)
    big_upload = build_upload_request("/upload", big_body)
    results.append(run_concurrent("Big uploads 64KB x10", big_upload, 10, expect_status_prefix="HTTP/1.1 "))

    # 30 concurrent chunked posts
    chunks = [b"abcd" * 50, b"efgh" * 50]
    chunked_req = build_chunked_request("/upload", chunks)
    results.append(run_concurrent("Chunked uploads x30", chunked_req, 30, expect_status_prefix="HTTP/1.1 "))

    upload_name = f"stress_{random.randint(1, 1_000_000)}.txt"
    upload_body = b"hello-delete"
    # Upload directly to /upload/<filename> using standard POST
    upload_req = build_upload_request(f"/upload/{upload_name}", upload_body)
    # Delete using standard DELETE to the same path (no CGI)
    delete_req = f"DELETE /upload/{upload_name} HTTP/1.1\r\n" "Host: localhost\r\n" "\r\n"
    up_res = run_test("Upload for delete", upload_req, expect_status_prefix="HTTP/1.1 ")
    results.append(up_res)
    if up_res.ok:
        results.append(run_test("Delete uploaded file", delete_req, expect_status_prefix="HTTP/1.1 "))

    return results


def wait_for_server():
    """
    Polls the server until it accepts a connection or timeout.
    """
    deadline = time.time() + SERVER_START_TIMEOUT
    while time.time() < deadline:
        try:
            with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
                s.settimeout(0.5)
                s.connect((HOST, PORT))
            return True
        except Exception:
            time.sleep(0.1)
    return False


def main():
    print(f"Starting webserv: {' '.join(WEBSERV_CMD)}")

    server_proc = subprocess.Popen(
        WEBSERV_CMD,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    try:
        if not wait_for_server():
            print("❌ Server did not start or is not listening on 127.0.0.1:8080")
            server_proc.terminate()
            stdout, stderr = server_proc.communicate(timeout=2)
            print("---- webserv stdout ----")
            print(stdout.decode(errors="ignore"))
            print("---- webserv stderr ----")
            print(stderr.decode(errors="ignore"))
            sys.exit(1)

        print("✅ Server is up, running tests...\n")

        tests = []

        # 1) Minimal valid GET /
        tests.append(
            run_test(
                "GET / (minimal)",
                "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",
                expect_status_prefix="HTTP/1.1 ",
            )
        )

        # 2) GET / without Host header (should likely be 400 for HTTP/1.1)
        tests.append(
            run_test(
                "GET / without Host",
                "GET / HTTP/1.1\r\n\r\n",
                # If you enforce Host, you might expect 400:
                # expect_status_prefix="HTTP/1.1 400"
                expect_status_prefix=None,  # just record what happens
            )
        )

        # 3) HEAD /
        tests.append(
            run_test(
                "HEAD /",
                "HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n",
                expect_status_prefix="HTTP/1.1 ",
            )
        )

        # 4) Nonexistent path
        tests.append(
            run_test(
                "GET /does_not_exist",
                "GET /does_not_exist HTTP/1.1\r\nHost: localhost\r\n\r\n",
                # Often 404:
                # expect_status_prefix="HTTP/1.1 404"
                expect_status_prefix=None,
            )
        )

        # 5) Malformed request line
        tests.append(
            run_test(
                "Bad request line",
                "GET / HTTP/1.1 garbage\r\nHost: localhost\r\n\r\n",
                # Might be 400:
                expect_status_prefix=None,
            )
        )

        # 6) HTTP/1.0 request
        tests.append(
            run_test(
                "GET / HTTP/1.0",
                "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n",
                expect_status_prefix="HTTP/1.0 ",
            )
        )

        # 7) Simple POST with small body
        tests.append(
            run_test(
                "POST /echo small body",
                "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nHello",
                expect_status_prefix="HTTP/1.1 ",
            )
        )

        # 8) Chunked POST (if your server supports TE: chunked for requests)
        tests.append(
            run_test(
                "POST / chunked body",
                ("POST / HTTP/1.1\r\n" "Host: localhost\r\n" "Transfer-Encoding: chunked\r\n\r\n" "4\r\nTest\r\n0\r\n\r\n"),
                expect_status_prefix="HTTP/1.1 ",
            )
        )

        # 9) Very long header
        long_hdr_val = "X" * 8000
        tests.append(
            run_test(
                "GET / with long header",
                "GET / HTTP/1.1\r\nHost: localhost\r\nX-Long: " + long_hdr_val + "\r\n\r\n",
                expect_status_prefix=None,
            )
        )

        # 10) Pipelined requests on same connection (2 GETs)
        pipelined_req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        tests.append(
            run_test(
                "Pipelined 2x GET /",
                pipelined_req,
                expect_status_prefix="HTTP/1.1 ",
            )
        )

        # Print results
        print("============ TEST RESULTS ============\n")
        for t in tests:
            status = "✅" if t.ok else "❌"
            print(f"{status} {t.name}")
            if t.status_line:
                print(f"   Status: {t.status_line}")
            if t.error:
                print(f"   Error:  {t.error}")
            if t.preview:
                print(f"   Body preview: {t.preview}")
            print()

        print("============ STRESS SUITE ============\n")
        stress_results = run_stress_suite()
        for t in stress_results:
            status = "✅" if t.ok else "❌"
            print(f"{status} {t.name}")
            if t.status_line:
                print(f"   Status: {t.status_line}")
            if t.error:
                print(f"   Error:  {t.error}")
            if t.preview:
                print(f"   Body preview: {t.preview}")
            print()

    finally:
        # Try to terminate the server cleanly
        server_proc.terminate()
        try:
            stdout, stderr = server_proc.communicate(timeout=2)
        except Exception:
            server_proc.kill()
            stdout, stderr = server_proc.communicate()

        print("============ webserv stdout ============")
        try:
            print(stdout.decode(errors="ignore"))
        except Exception:
            pass

        print("============ webserv stderr ============")
        try:
            print(stderr.decode(errors="ignore"))
        except Exception:
            pass


if __name__ == "__main__":
    main()
