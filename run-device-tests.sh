#!/bin/bash

# Usage: run-device-tests.sh [--build] [--no-flash]
#   --build    compile the test firmware first
#   --no-flash skip flashing (assumes the test firmware is already running)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FIRMWARE="$SCRIPT_DIR/build/pio-i2s-tests.elf"

BUILD=false
FLASH=true

while [ $# -gt 0 ]; do
    case "$1" in
        --build)
            BUILD=true
            shift
            ;;
        --no-flash)
            FLASH=false
            shift
            ;;
        *)
            echo "Unknown option: $1" >&2
            echo "Usage: run-device-tests.sh [--build] [--no-flash]" >&2
            exit 1
            ;;
    esac
done

if [ "$BUILD" = true ]; then
    "$SCRIPT_DIR/compile.sh"
fi

if [ "$FLASH" = true ]; then
    if [ ! -f "$FIRMWARE" ]; then
        echo "Firmware not found: $FIRMWARE" >&2
        echo "Run with --build to compile it first." >&2
        exit 1
    fi
    "$SCRIPT_DIR/flash-firmware.sh" "$FIRMWARE"
fi

exec python3 "$SCRIPT_DIR/test/device/read-device-test-results.py"
