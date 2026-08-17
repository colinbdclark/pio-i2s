/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_CLOCK_DIVISION_TESTS_H
#define TEST_CLOCK_DIVISION_TESTS_H

#include "unity.h"
#include "pio-i2s.h"
#include "test-support.h"

#ifdef PIOI2S_TEST_HOST
#include "hardware/clocks.h"
#endif

static void testCalculateClockDivision(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    self.config = &config;

    TEST_ASSERT_EQUAL_FLOAT(TEST_EXPECTED_CLOCK_DIV, PioI2S_calculateClockDivision(&self));
}

static void testCalculateClockDivisionSixteenBit(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    self.config = &config;
    config.bitDepth = 16;

    // 153600000 / (48000 * 2 * 16 * 2) = 50.0 exactly.
    TEST_ASSERT_EQUAL_FLOAT(50.0f, PioI2S_calculateClockDivision(&self));
}

#ifdef PIOI2S_TEST_HOST
static void testCalculateClockDivisionAtLowerSystemClock(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    self.config = &config;

    mockClockSetSysSpeed(122880000);

    // 122880000 / (48000 * 2 * 32 * 2) = 20.0 exactly.
    TEST_ASSERT_EQUAL_FLOAT(20.0f, PioI2S_calculateClockDivision(&self));
}
#endif

static inline void runClockDivisionTests(void) {
    RUN_TEST(testCalculateClockDivision);
    RUN_TEST(testCalculateClockDivisionSixteenBit);
#ifdef PIOI2S_TEST_HOST
    RUN_TEST(testCalculateClockDivisionAtLowerSystemClock);
#endif
}

#endif // TEST_CLOCK_DIVISION_TESTS_H
