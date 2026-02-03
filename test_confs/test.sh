#!/bin/bash

WEBSERV=./webserv
VALID_DIR=test_confs/confs_valid
INVALID_DIR=test_confs/confs_invalid

GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m"

run_test() {
    local config="$1"
    local should_start="$2"

    echo "▶ Testing $config"

    tmpfile=$(mktemp)

    # Run server in background, capture output to file
    "$WEBSERV" "$config" >"$tmpfile" 2>&1 &
    pid=$!

    # Give it time to start
    sleep 1

    if ps -p "$pid" > /dev/null 2>&1; then
        # Server is running -> startup succeeded
        kill "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null  # reap it (avoid zombies)

        if [ "$should_start" = "yes" ]; then
            echo -e "${GREEN}✔ PASS${NC} (server started)"
        else
            echo -e "${RED}✘ FAIL${NC} (server should NOT start)"
            echo "  Output:"
            sed 's/^/  /' "$tmpfile"
        fi
    else
        # Server exited -> startup failed
        output=$(cat "$tmpfile")

        if [ "$should_start" = "no" ]; then
            echo -e "${GREEN}✔ PASS${NC} (failed as expected)"
            echo "  Error:"
            echo "$output" | sed 's/^/  /'
        else
            echo -e "${RED}✘ FAIL${NC} (server did not start)"
            echo "  Error:"
            echo "$output" | sed 's/^/  /'
        fi
    fi

    rm -f "$tmpfile"
    echo
}

echo "===== VALID CONFIGS ====="
for conf in "$VALID_DIR"/*.conf; do
    run_test "$conf" "yes"
done

echo "===== INVALID CONFIGS ====="
for conf in "$INVALID_DIR"/*.conf; do
    run_test "$conf" "no"
done
