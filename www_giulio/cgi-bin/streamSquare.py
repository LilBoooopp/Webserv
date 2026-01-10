#!/usr/bin/env python3
import os
import sys
import time
import random
import signal

# Ignore SIGPIPE so we exit quietly if the client goes away
signal.signal(signal.SIGPIPE, signal.SIG_IGN)

# Turn off buffering to stream immediately
sys.stdout.reconfigure(line_buffering=True)


def generate_square(n):
    return "\n".join(" ".join(f"{random.randint(0,9)}" for _ in range(n)) for _ in range(n))


# CGI headers (no Content-Length, so the server can stream/chunk)
try:
    print("Status: 200 OK")
    print("Content-Type: text/plain; charset=utf-8")
    print()  # end of headers
except BrokenPipeError:
    sys.exit(0)

SIZE = 5  # square size
DELAY = 0.5  # seconds between frames

try:
    while True:
        try:
            square = generate_square(SIZE)
            print(square)
            print("---")  # separator between frames (optional)
            sys.stdout.flush()
        except BrokenPipeError:
            sys.exit(0)
        time.sleep(DELAY)
except BrokenPipeError:
    # Client disconnected before loop
    sys.exit(0)
