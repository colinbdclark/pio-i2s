#!/bin/sh

# Usage: ./docker-compile.sh [extra cmake configure args...]

set -eu

SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" && pwd)

cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build-docker" "$@"
cmake --build "$SCRIPT_DIR/build-docker" -j
