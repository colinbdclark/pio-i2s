#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" && pwd)

rm -rf "$SCRIPT_DIR/build" "$SCRIPT_DIR/build-docker"
