#!/usr/bin/env python3
import os
import sys
import re


def respond(status_code, body, content_type="text/plain"):
    sys.stdout.write(f"Status: {status_code}\r\n")
    sys.stdout.write(f"Content-Type: {content_type}\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)


def increment_filename(filepath):
    """
    If file exists, add/increment a _number suffix.
    E.g., map.txt -> map_0.txt, map_9.txt -> map_10.txt
    """
    if not os.path.exists(filepath):
        return filepath

    base, ext = os.path.splitext(filepath)
    match = re.search(r"_(\d+)$", base)

    if match:
        # Already has _number suffix, increment it
        num = int(match.group(1))
        base_without_num = base[: match.start()]
        counter = num + 1
        while os.path.exists(f"{base_without_num}_{counter}{ext}"):
            counter += 1
        return f"{base_without_num}_{counter}{ext}"
    else:
        # No suffix yet, add _0
        counter = 0
        while os.path.exists(f"{base}_{counter}{ext}"):
            counter += 1
        return f"{base}_{counter}{ext}"


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

    mode = os.environ.get("HTTP_X_MODE", "create").lower()
    is_directory = filename.endswith("/")

    filename = os.path.normpath(filename)

    # Prevent directory traversal (but allow "." to refer to root)
    if filename.startswith(".."):
        respond("400 Bad Request", "Invalid path\n")
        return

    upload_dir = os.path.normpath(os.path.join(os.path.dirname(__file__), "../ressources/uploads"))
    try:
        os.makedirs(upload_dir, exist_ok=True)
    except Exception as e:
        respond("500 Internal Server Error", f"Failed to prepare upload dir: {e}\n")
        return

    if is_directory:
        dest = os.path.join(upload_dir, filename)
        try:
            os.makedirs(dest, exist_ok=True)
        except Exception as e:
            respond("500 Internal Server Error", f"Failed to create directory: {e}\n")
            return
        respond("200 OK", f"Created directory {filename}\n")
        return

    # Otherwise, treat as file upload
    dest = os.path.join(upload_dir, filename)

    # If mode is "create" and file exists, increment filename
    if mode == "create":
        dest = increment_filename(dest)
        # Extract the new filename for response
        filename = os.path.relpath(dest, upload_dir)

    dest_dir = os.path.dirname(dest)
    try:
        os.makedirs(dest_dir, exist_ok=True)
    except Exception as e:
        respond("500 Internal Server Error", f"Failed to create subdirectory: {e}\n")
        return

    data = sys.stdin.buffer.read(content_length)

    try:
        file_mode = "ab" if mode == "append" else "wb"
        with open(dest, file_mode) as f:
            f.write(data)
    except Exception as e:
        respond("500 Internal Server Error", f"Failed to write file: {e}\n")
        return

    respond("200 OK", f"Stored as {filename}\n")


if __name__ == "__main__":
    main()
