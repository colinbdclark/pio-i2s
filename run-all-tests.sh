#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "========================================"
echo "  Checking the generated PIO header"
echo "========================================"
"$SCRIPT_DIR/update-pio-header.sh" --check
echo "$(basename "$SCRIPT_DIR")/include/pio-i2s-out.pio.h is up to date."

echo ""
echo "========================================"
echo "  Host tests"
echo "========================================"
"$SCRIPT_DIR/run-host-tests.sh"

echo ""
echo "========================================"
echo "  Device tests"
echo "========================================"
"$SCRIPT_DIR/run-device-tests.sh" --build
