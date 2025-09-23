#!/usr/bin/env bash
set -euo pipefail

# --- config ---
WS_BIN="${WS_BIN:-./webserv}"
WS_ADDR="${WS_ADDR:-127.0.0.1}"
WS_PORT="${WS_PORT:-8080}"
WS_URL="http://${WS_ADDR}:${WS_PORT}"
TMP_DIR="${TMP_DIR:-/tmp/webserv-smoke}"
TIMEOUT="${TIMEOUT:-3}"

# --- helpers ---
RED=$'\033[31m'; GRN=$'\033[32m'; YLW=$'\033[33m'; BLU=$'\033[34m'; RST=$'\033[0m'
pass(){ echo "${GRN}✔ PASS${RST} $*"; }
fail(){ echo "${RED}✘ FAIL${RST} $*"; exit 1; }
info(){ echo "${BLU}ℹ${RST} $*"; }

need_bin() {
  command -v "$1" >/dev/null 2>&1 || fail "missing dependency: $1"
}

curl_f() { curl --max-time "${TIMEOUT}" -sSf "$@" ; }

# --- preflight ---
need_bin curl
need_bin nc
need_bin grep
need_bin awk
mkdir -p "${TMP_DIR}"

# --- (re)start server if not running ---
PID_FILE="${TMP_DIR}/webserv.pid"
if [[ -f "${PID_FILE}" ]] && ps -p "$(cat "${PID_FILE}")" >/dev/null 2>&1; then
  info "Using existing webserv (pid $(cat "${PID_FILE}"))"
else
  info "Starting ${WS_BIN} on ${WS_ADDR}:${WS_PORT}"
  "${WS_BIN}" &
  echo $! > "${PID_FILE}"
  sleep 0.25
fi

# --- ensure www/index.html exists ---
mkdir -p www
# If you want *no* trailing newline, uncomment the printf line and comment the echo line:
# printf "<h1>OK</h1>" > www/index.html
echo "<h1>OK</h1>" > www/index.html

# --- tests ---
TOTAL=0; OK=0

test_case() {
  local name="$1"; shift
  TOTAL=$((TOTAL+1))
  if "$@"; then pass "${name}"; OK=$((OK+1)); else fail "${name}"; fi
}

# 1) GET / headers + body (use GET, not HEAD)
test_case "GET / returns index.html and Content-Type: text/html" bash -c '
  H=$(curl -sS -D - -o /dev/null "'"${WS_URL}/"'" | tr -d "\r")
  B=$(curl -s "'"${WS_URL}/"'")
  echo "$H" | grep -q "^HTTP/1.1 200" &&
  echo "$H" | grep -qi "^Content-Type: *text/html" &&
  echo "$B" | tr -d "\n" | grep -qx "<h1>OK</h1>"
'

# 2) 404
test_case "GET /nope -> 404" bash -c '
  curl -sS -D - -o /dev/null "'"${WS_URL}/nope"'" | tr -d "\r" | grep -q "^HTTP/1.1 404"
'

# 3) HTTP/1.1 requires Host (400 if missing)
test_case "Missing Host header -> 400" bash -c '
  printf "GET / HTTP/1.1\r\n\r\n" | nc -w '"${TIMEOUT}"' '"${WS_ADDR}"' '"${WS_PORT}"' | tr -d "\r" | head -n1 | grep -q "^HTTP/1.1 400"
'

# 4) POST with Content-Length
test_case "POST (Content-Length) echoes size" bash -c '
  printf "abc123" > "'"${TMP_DIR}"'/b.bin"
  curl -sS -X POST --data-binary @"'"${TMP_DIR}"'/b.bin" "'"${WS_URL}/echo"'" | grep -q "received 6 bytes"
'

# 5) POST chunked (two chunks: 4 + 3)
test_case "POST (chunked) echoes size" bash -c '
  printf '"'"'POST /echo HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWXYZ\r\n3\r\n123\r\n0\r\n\r\n'"'"' \
  | nc -w '"${TIMEOUT}"' '"${WS_ADDR}"' '"${WS_PORT}"' | grep -q "received 7 bytes"
'

# 6) TE + CL conflict -> 400
test_case "TE+CL conflict -> 400" bash -c '
  printf '"'"'POST /echo HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\n0\r\n\r\n'"'"' \
  | nc -w '"${TIMEOUT}"' '"${WS_ADDR}"' '"${WS_PORT}"' | tr -d "\r" | head -n1 | grep -q "^HTTP/1.1 400"
'

# 7) Header cap (oversized head -> 431)
test_case "Header too large -> 431" bash -c '
  big=$(python - <<PY 2>/dev/null || perl -e '"'"'print "a"x20000'"'"'
s="a"*20000
print s
PY
)
  { printf "GET / HTTP/1.1\r\nHost: x\r\nX-Long: %s\r\n\r\n" "$big"; } \
  | nc -w '"${TIMEOUT}"' '"${WS_ADDR}"' '"${WS_PORT}"' | tr -d "\r" | head -n1 | grep -q "^HTTP/1.1 431"
'

echo
echo "${YLW}Result:${RST} ${OK}/${TOTAL} tests passed."
exit $(( OK == TOTAL ? 0 : 1 ))
