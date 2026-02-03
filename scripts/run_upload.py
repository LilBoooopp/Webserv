#!/usr/bin/env python3
"""
Small helper to invoke a CGI script (like www_giulio/cgi-bin/upload.py)
with the minimal environment used by the server for testing.

Usage examples:
  # inline body
  python3 scripts/run_upload.py --upload-path hello_manual.txt --body "Hello from script"

  # from a file
  python3 scripts/run_upload.py --upload-path hello_manual.txt --body-file /tmp/body.bin

  # create directory (no body required)
  python3 scripts/run_upload.py --upload-path newdir/ --no-body

The script prints CGI stdout and stderr to the terminal.
"""

import os
import sys
import argparse
import subprocess


def main():
    parser = argparse.ArgumentParser(description="Run a CGI script with upload env vars")
    parser.add_argument('--cgi', default='www_giulio/cgi-bin/upload.py', help='Path to CGI script')
    parser.add_argument('--upload-path', required=True, help='Value for X-Upload-Path header (relative path)')
    parser.add_argument('--mode', choices=['replace','create','append'], help='X-Mode header')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--body-file', help='Read body from this file')
    group.add_argument('--body', help='Body literal (no trailing newline unless included)')
    parser.add_argument('--no-body', action='store_true', help='Send no body (useful for creating dirs)')

    args = parser.parse_args()

    if args.no_body:
        body = b''
    elif args.body_file:
        try:
            with open(args.body_file, 'rb') as f:
                body = f.read()
        except Exception as e:
            print('Failed to read body file:', e, file=sys.stderr)
            return 2
    elif args.body is not None:
        body = args.body.encode()
    else:
        # default: empty body
        body = b''

    env = os.environ.copy()
    # CGI script expects HTTP_X_UPLOAD_PATH env var
    env['HTTP_X_UPLOAD_PATH'] = args.upload_path
    env['REQUEST_METHOD'] = 'POST'
    env['CONTENT_LENGTH'] = str(len(body))
    if args.mode:
        env['HTTP_X_MODE'] = args.mode

    # Ensure python can find and execute the script path
    cgi_path = args.cgi
    if not os.path.isabs(cgi_path):
        cgi_path = os.path.join(os.getcwd(), cgi_path)

    if not os.path.exists(cgi_path):
        print('CGI script not found at', cgi_path, file=sys.stderr)
        return 3

    # Run the CGI script using the same Python interpreter
    try:
        proc = subprocess.run([sys.executable, cgi_path], input=body, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except Exception as e:
        print('Failed to run CGI:', e, file=sys.stderr)
        return 4

    # Print stdout (CGI writes HTTP-like headers + body)
    try:
        sys.stdout.buffer.write(proc.stdout)
    except Exception:
        # fallback for weird environments
        sys.stdout.write(proc.stdout.decode(errors='replace'))

    if proc.stderr:
        sys.stderr.write('\n-- CGI stderr --\n')
        try:
            sys.stderr.buffer.write(proc.stderr)
        except Exception:
            sys.stderr.write(proc.stderr.decode(errors='replace'))

    return proc.returncode


if __name__ == '__main__':
    sys.exit(main())
