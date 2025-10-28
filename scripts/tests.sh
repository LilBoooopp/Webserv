#!/usr/bin/env bash
set -euo pipefail

WS_BIN="${WS_BIN:-./webserv}"
WS_ADDR="${WS_ADDR:-127.0.0.1}"
WS_PORT="${WS_PORT:-8080}"
WS_URL="http://${WS_ADDR}:${WS_PORT}"
TMP_DIR="${TMP_DIR:-/tmp/webserv-smoke}"
TIMEOUT="${TIMEOUT:-3}"

RED=$'\033[31m'; GRN=$'\033[32m'; YLW=$'\033[33m'; BLU=$'\033[34m'; RST=$'\033[0m'
pass(){ echo "${GRN}✔ PASS${RST} $*"; }
fail(){ echo "${RED}✘ FAIL${RST} $*"; exit 1; }
info(){ echo "${BLU}ℹ${RST} $*"; }

need_bin(){ command -v "$1" >/dev/null 2>&1 || fail "missing dependency: $1"; }
curl_f(){ curl --max-time "${TIMEOUT}" -sSf "$@"; }

need_bin curl; need_bin nc; need_bin grep; need_bin awk; mkdir -p "${TMP_DIR}"

PID_FILE="${TMP_DIR}/webserv.pid"
PG_FILE="${TMP_DIR}/webserv.pgid"

cleanup(){
  if [[ -f "${PG_FILE}" ]]; then
    PGID=$(cat "${PG_FILE}") || true
    kill -TERM -"${PGID}" 2>/dev/null || true
    sleep 0.3
    kill -KILL -"${PGID}" 2>/dev/null || true
  elif [[ -f "${PID_FILE}" ]]; then
    PID=$(cat "${PID_FILE}") || true
    kill -TERM "${PID}" 2>/dev/null || true
    sleep 0.3
    kill -KILL "${PID}" 2>/dev/null || true
  fi
  lsof -ti tcp:"${WS_PORT}" -sTCP:LISTEN 2>/dev/null | xargs -r kill -TERM 2>/dev/null || true
  rm -f "${PID_FILE}" "${PG_FILE}" || true
}
trap cleanup EXIT INT TERM

if [[ -f "${PID_FILE}" ]] && ps -p "$(cat "${PID_FILE}")" >/dev/null 2>&1; then
  info "Using existing webserv (pid $(cat "${PID_FILE}"))"
  PGID=$(ps -o pgid= -p "$(cat "${PID_FILE}")" | tr -d ' ')
  echo "${PGID}" > "${PG_FILE}"
else
  info "Starting ${WS_BIN} on ${WS_ADDR}:${WS_PORT}"
  setsid "${WS_BIN}" >/dev/null 2>&1 &
  PID=$!
  echo "${PID}" > "${PID_FILE}"
  echo "${PID}" > "${PG_FILE}"
  sleep 0.25
fi

mkdir -p www
echo "<h1>OK</h1>" > www/index.html

TOTAL=0; OK=0
test_case(){ local name="$1"; shift; TOTAL=$((TOTAL+1)); if "$@"; then pass "${name}"; OK=$((OK+1)); else fail "${name}"; fi; }

test_case "GET / returns index.html and Content-Type: text/html" bash -c '
  H=$(curl -sS -D - -o /dev/null "'"${WS_URL}/"'" | tr -d "\r")
  B=$(curl -s "'"${WS_URL}/"'")
  echo "$H" | grep -q "^HTTP/1.1 200" &&
  echo "$H" | grep -qi "^Content-Type: *text/html" &&
  echo "$B" | tr -d "\n" | grep -qx "<h1>OK</h1>"
'

test_case "GET /nope -> 404" bash -c '
  curl -sS -D - -o /dev/null "'"${WS_URL}/nope"'" | tr -d "\r" | grep -q "^HTTP/1.1 404"
'

test_case "Missing Host header -> 400" bash -c '
  printf "GET / HTTP/1.1\r\n\r\n" | nc -w '"${TIMEOUT}"' '"${WS_ADDR}"' '"${WS_PORT}"' | tr -d "\r" | head -n1 | grep -q "^HTTP/1.1 400"
'

test_case "POST (Content-Length) echoes size" bash -c '
  printf "abc123" > "'"${TMP_DIR}"'/b.bin"
  curl -sS -X POST --data-binary @"'"${TMP_DIR}"'/b.bin" "'"${WS_URL}/echo"'" | grep -q "received 6 bytes"
'

test_case "POST (chunked) echoes size" bash -c '
  printf '"'"'POST /echo HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWXYZ\r\n3\r\n123\r\n0\r\n\r\n'"'"' \
  | nc -w '"${TIMEOUT}"' '"${WS_ADDR}"' '"${WS_PORT}"' | grep -q "received 7 bytes"
'

test_case "TE+CL conflict -> 400" bash -c '
  printf '"'"'POST /echo HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\n0\r\n\r\n'"'"' \
  | nc -w '"${TIMEOUT}"' '"${WS_ADDR}"' '"${WS_PORT}"' | tr -d "\r" | head -n1 | grep -q "^HTTP/1.1 400"
'

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
