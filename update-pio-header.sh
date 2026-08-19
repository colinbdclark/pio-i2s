#!/bin/sh

# Usage: ./update-pio-header.sh [--check]
#   --check  fail if the generated header is out of date instead of rewriting it

set -eu

SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" && pwd)
PIOASM_DIR="$SCRIPT_DIR/build/pioasm-tool"
PIOASM="$PIOASM_DIR/pioasm"
PIO_SOURCE="$SCRIPT_DIR/src/pio-i2s-out.pio"
HEADER="$SCRIPT_DIR/include/pio-i2s-out.pio.h"

CHECK=false
if [ $# -gt 0 ]; then
    if [ "$1" = "--check" ]; then
        CHECK=true
    else
        echo "Unknown option: $1" >&2
        echo "Usage: $0 [--check]" >&2
        exit 1
    fi
fi

if [ ! -x "$PIOASM" ]; then
    cmake -S "$SCRIPT_DIR/lib/pico-sdk/tools/pioasm" -B "$PIOASM_DIR" > /dev/null
    cmake --build "$PIOASM_DIR" > /dev/null
fi

if [ "$CHECK" = false ]; then
    "$PIOASM" -o c-sdk "$PIO_SOURCE" "$HEADER"
    exit 0
fi

GENERATED=$(mktemp)
trap 'rm -f "$GENERATED"' EXIT
"$PIOASM" -o c-sdk "$PIO_SOURCE" "$GENERATED"

if ! diff -u "$HEADER" "$GENERATED"; then
    echo "" >&2
    echo "Error: $HEADER is out of date. Run $0 to regenerate it." >&2
    exit 1
fi
