/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/clocks.h"
#include "unity.h"
#include "led.h"
#include "all-tests.h"

#define TEST_BEGIN_MARKER "=== PIOI2S-TEST-BEGIN ==="

// 30 seconds, in microseconds.
static const uint32_t kHostConnectTimeout = 30u * 1000u * 1000u;

// 2 seconds, in microseconds.
static const uint32_t kResultReportInterval = 2u * 1000u * 1000u;

void setUp(void) {
    stdio_flush();
}

void tearDown(void) {
    testRunCleanups();
    stdio_flush();
}

static void waitForHostConnection(void) {
    uint32_t start = time_us_32();
    while (!stdio_usb_connected() &&
        time_us_32() - start < kHostConnectTimeout) {
        sleep_ms(10);
    }
}

static void printTestReport(int passed, int failures) {
    printf("%s\n", TEST_BEGIN_MARKER);
    printf("=== PIOI2S-TEST-RESULT pass=%d fail=%d ===\n", passed, failures);
}

static int runTestSuite(void) {
    UNITY_BEGIN();
    runAllTests();
    return UNITY_END();
}

static int countPassedTests(void) {
    return (int) Unity.NumberOfTests -
        (int) Unity.TestFailures - (int) Unity.TestIgnores;
}

static void signalResultsForever(int passed, int failures) {
    LED led;
    ledInit(&led);
    ledSet(&led, failures == 0);

    uint32_t lastReport = time_us_32();
    while (true) {
        sleep_ms(100);
        if (failures != 0) {
            ledToggle(&led);
        }
        if (stdio_usb_connected() &&
            time_us_32() - lastReport > kResultReportInterval) {
            printTestReport(passed, failures);
            lastReport = time_us_32();
        }
    }
}

int main(void) {
    set_sys_clock_khz(153600, true);
    stdio_init_all();

    waitForHostConnection();
    printf("\n%s\n", TEST_BEGIN_MARKER);

    int failures = runTestSuite();
    int passed = countPassedTests();

    printTestReport(passed, failures);
    signalResultsForever(passed, failures);
}
