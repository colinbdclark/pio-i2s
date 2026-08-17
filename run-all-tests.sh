#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "========================================"
echo "  Host tests"
echo "========================================"
"$SCRIPT_DIR/run-host-tests.sh"

echo ""
echo "========================================"
echo "  Device tests"
echo "========================================"
"$SCRIPT_DIR/run-device-tests.sh" --build
