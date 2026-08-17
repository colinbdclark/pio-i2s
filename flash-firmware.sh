#!/bin/sh

# Usage: ./flash-firmware.sh <firmware-file>.elf

set -eu

if [ $# -ne 1 ]; then
    echo "Error: No firmware file specified." >&2
    echo "Usage: $0 <firmware-file>.elf" >&2
    exit 1
fi

FIRMWARE=$1

if [ ! -f "$FIRMWARE" ]; then
    echo "Error: Firmware file not found: $FIRMWARE" >&2
    exit 1
fi

if [ -z "${PICO_OPENOCD_PATH:-}" ]; then
    echo "Error: PICO_OPENOCD_PATH is not set. It must point to a directory" >&2
    echo "containing an RP2350-compatible OpenOCD (see the README)." >&2
    exit 1
fi

OPENOCD="$PICO_OPENOCD_PATH/openocd"

if [ ! -x "$OPENOCD" ]; then
    echo "Error: OpenOCD not found at $OPENOCD" >&2
    exit 1
fi

if [ ! -d "$PICO_OPENOCD_PATH/scripts" ]; then
    echo "Error: OpenOCD scripts directory not found at $PICO_OPENOCD_PATH/scripts" >&2
    exit 1
fi

"$OPENOCD" \
    -s "$PICO_OPENOCD_PATH/scripts" \
    -f interface/cmsis-dap.cfg \
    -f target/rp2350.cfg \
    -c "adapter speed 5000" \
    -c "program $FIRMWARE verify reset exit"
