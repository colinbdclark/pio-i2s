/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#include "unity.h"
#include "pico/stdlib.h"
#include "all-tests.h"

void setUp(void) {
    mockReset();
}

void tearDown(void) {
    testRunCleanups();
}

int main(void) {
    UNITY_BEGIN();

    runAllTests();

    return UNITY_END();
}
