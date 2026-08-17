#!/bin/bash

# Usage: ./run-host-tests.sh [extra cmake configure args...]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build/host-tests"

cmake -S "$SCRIPT_DIR/test" -B "$BUILD_DIR" "$@"
cmake --build "$BUILD_DIR" -j
ctest --test-dir "$BUILD_DIR" --output-on-failure
