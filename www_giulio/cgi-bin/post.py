#!/usr/bin/env python3
import os
import requests

# --- Configuration ---
TARGET_URL = "http://127.0.0.1:8000/uploads/postBytes/test_1mb.txt"
FILE_SIZE_MB = 1
FILENAME = "temp_1mb_payload.bin"

def generate_file():
    """Generates a file of specific size with random data."""
    with open(FILENAME, "wb") as f:
        f.write(os.urandom(FILE_SIZE_MB * 1024 * 1024))

def perform_upload():
    try:
        # Generate the dummy file
        generate_file()

        # Open the file in binary mode
        with open(FILENAME, "rb") as f:
            # NOTE: usage of 'data=f' instead of 'files=' forces 
            # Chunked Transfer Encoding in Python Requests library 
            # if the file is opened as a stream.
            print(f"Uploading {FILE_SIZE_MB}MB to {TARGET_URL}...")
            
            # Sending the POST request
            response = requests.post(TARGET_URL, data=f)

            print(f"--- Server Response ---")
            print(f"Status: {response.status_code}")
            print(f"Body: {response.text}")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Cleanup
        if os.path.exists(FILENAME):
            os.remove(FILENAME)

if __name__ == "__main__":
    # CGI Header (Standard Output)
    print("Content-Type: text/plain\r\n\r\n")

    # Check if triggered via GET (or run manually)
    method = os.environ.get("REQUEST_METHOD", "GET")
    
    if method == "GET":
        perform_upload()
    else:
        print("Error: This script only accepts GET requests to trigger the POST.")
