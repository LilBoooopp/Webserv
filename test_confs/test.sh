#!/bin/bash

WEBSERV=../webserv
VALID_DIR=./confs_valid
INVALID_DIR=./confs_invalid

GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m"

run_test() {
    local config="$1"
    local should_start="$2"

    echo "▶ Testing $config"

    # Run server, capture output
    output=$($WEBSERV "$config" 2>&1 &)
    pid=$!

    # Give it time to start
    sleep 1

    if ps -p $pid > /dev/null 2>&1; then
        # Server is running
        kill $pid 2>/dev/null

        if [ "$should_start" = "yes" ]; then
            echo -e "${GREEN}✔ PASS${NC} (server started)"
        else
            echo -e "${RED}✘ FAIL${NC} (server should NOT start)"
        fi
    else
        # Server exited
        if [ "$should_start" = "no" ]; then
            echo -e "${GREEN}✔ PASS${NC} (failed as expected)"
            echo "  Error:"
            echo "  $output"
        else
            echo -e "${RED}✘ FAIL${NC} (server did not start)"
            echo "  Error:"
            echo "  $output"
        fi
    fi

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
