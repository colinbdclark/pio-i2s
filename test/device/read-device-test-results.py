#!/usr/bin/env python3
"""
Reads the PioI2S on-device test results from the Pico's USB serial port.

Usage:
    read-device-test-results.py [--timeout SECONDS] [--port-timeout SECONDS]

The test firmware (see test/device) reports its results over USB CDC serial.
This script:

  1. Finds the Pico's serial port by polling the candidate ports and
     selecting the first one that sends the test firmware's begin marker.
  2. Reads the test output, waits for the firmware's machine-readable result
     line, prints the full output, and exits non-zero if any test failed or the
     tests timed out.
"""

import argparse
import glob
import os
import re
import select
import sys
import termios
import time

BEGIN_MARKER = "=== PIOI2S-TEST-BEGIN ==="
RESULT_RE = re.compile(r"=== PIOI2S-TEST-RESULT pass=(\d+) fail=(\d+) ===")


def fail(message):
    print(f"ERROR: {message}", file=sys.stderr)
    sys.exit(1)


def candidate_ports():
    """Returns the USB serial devices that might be the Pico."""
    if sys.platform == "darwin":
        ports = glob.glob("/dev/cu.usbmodem*")
    else:
        ports = glob.glob("/dev/ttyACM*")
        ports += glob.glob("/dev/serial/by-id/*")
    # Deduplicate, preserving order.
    seen = set()
    result = []
    for port in sorted(ports):
        if port not in seen:
            seen.add(port)
            result.append(port)
    return result


def open_port(port):
    """Opens the port at 115200 8N1 in raw, non-blocking mode.

    Opening the device asserts DTR, which is the signal the test firmware
    waits for before running the tests (stdio_usb_connected).
    """
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0                                   # iflag
    attrs[1] = 0                                   # oflag
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # cflag
    attrs[3] = 0                                   # lflag (raw, no echo)
    attrs[4] = termios.B115200                     # ispeed
    attrs[5] = termios.B115200                     # ospeed
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


class PortReader:
    def __init__(self, fd):
        self.fd = fd
        self.buffer = ""

    def read(self, timeout):
        """Reads any available data and returns it as text (possibly empty)."""
        readable, _, _ = select.select([self.fd], [], [], timeout)
        if not readable:
            return ""
        try:
            data = os.read(self.fd, 4096)
        except OSError:
            return ""
        text = data.decode("utf-8", errors="replace")
        self.buffer += text
        return text


def find_device_port(timeout):
    """Finds the Pico's port by identifhing the first candidate that sends the
    begin marker after being opened. Ports are retried until the deadline,
    because the device re-enumerates after a reset and may not be ready
    on the first attempt. Returns (fd, reader) or (None, None)."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        for port in candidate_ports():
            try:
                fd = open_port(port)
            except OSError:
                continue

            # The device prints the begin marker in response to our open
            # (which asserts DTR), so give it a few seconds to respond.
            reader = PortReader(fd)
            marker_deadline = time.time() + 3.0
            while time.time() < marker_deadline:
                reader.read(0.2)
                if BEGIN_MARKER in reader.buffer:
                    print(f"Found the Pico on {port}")
                    return fd, reader
            os.close(fd)
        time.sleep(0.25)
    return None, None


def run_tests(reader, timeout):
    """Reads test output until the result line arrives or time runs out."""
    log = ""
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = reader.read(0.5)
        if text:
            print(text, end="", flush=True)
            log += text
            if RESULT_RE.search(log):
                return log
    return log


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, default=120.0,
        help="maximum seconds to wait for test results (default: 120)")
    parser.add_argument("--port-timeout", type=float, default=20.0,
        help="maximum seconds to wait for the Pico's serial port "
             "(default: 20)")
    args = parser.parse_args()

    print("Waiting for the Pico's serial port...")
    fd, reader = find_device_port(args.port_timeout)
    if fd is None:
        fail("No Pico test port found. Is the device connected via USB?")

    try:
        log = run_tests(reader, args.timeout)
    finally:
        os.close(fd)

    match = RESULT_RE.search(log)
    if not match:
        fail(f"No test result received within {args.timeout} seconds.")

    passes, failures = int(match.group(1)), int(match.group(2))
    if failures != 0:
        fail(f"{failures} test(s) failed.")

    print(f"\nAll {passes} on-device tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
