#!/usr/bin/env python3
import os
import sys
import cgi


def respond(status_code, body, content_type="text/plain"):
    sys.stdout.write(f"Status: {status_code}\r\n")
    sys.stdout.write(f"Content-Type: {content_type}\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)


def main():
    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
    except ValueError:
        content_length = 0

    # Get path from X-Upload-Path header (passed via HTTP_X_UPLOAD_PATH env var)
    filename = os.environ.get("HTTP_X_UPLOAD_PATH", "")

    if not filename:
        respond("400 Bad Request", "Missing X-Upload-Path header\n")
        return

    # Normalize path but allow subdirectories (don't use basename which strips dirs)
    filename = os.path.normpath(filename)

    # Prevent directory traversal
    if filename.startswith("..") or filename.startswith("/"):
        respond("400 Bad Request", "Invalid path\n")
        return

    upload_dir = os.path.normpath(os.path.join(os.path.dirname(__file__), "../ressources/uploads"))
    try:
        os.makedirs(upload_dir, exist_ok=True)
    except Exception as e:
        respond("500 Internal Server Error", f"Failed to prepare upload dir: {e}\n")
        return

    # Create subdirectories if needed
    dest = os.path.join(upload_dir, filename)
    dest_dir = os.path.dirname(dest)
    try:
        os.makedirs(dest_dir, exist_ok=True)
    except Exception as e:
        respond("500 Internal Server Error", f"Failed to create subdirectory: {e}\n")
        return

    data = sys.stdin.buffer.read(content_length)

    dest = os.path.join(upload_dir, filename)
    try:
        with open(dest, "wb") as f:
            f.write(data)
    except Exception as e:
        respond("500 Internal Server Error", f"Failed to write file: {e}\n")
        return

    respond("200 OK", f"Stored as {filename}\n")


if __name__ == "__main__":
    main()
