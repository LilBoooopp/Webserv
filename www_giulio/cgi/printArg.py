import os
from urllib.parse import parse_qs, unquote

qs = os.environ.get("QUERY_STRING", "")
params = parse_qs(qs)
arg = params.get("arg", [""])[0]
arg = unquote(arg)  # decode %2F → /

# If arg is empty → default to "/"
if not arg:
    arg = "/"

print("Content-Type: text/html")
print()
print(arg)
