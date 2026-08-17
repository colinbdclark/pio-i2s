/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_CONVERSION_TESTS_H
#define TEST_CONVERSION_TESTS_H

#include <math.h>
#include <stdint.h>
#include "unity.h"
#include "pio-i2s.h"

static void testFloatToInt32Zero(void) {
    TEST_ASSERT_EQUAL_INT32(0, PioI2s_floatToInt32(0.0f));
}

static void testFloatToInt32Positive(void) {
    TEST_ASSERT_EQUAL_INT32(1073741824, PioI2s_floatToInt32(0.5f));
    TEST_ASSERT_EQUAL_INT32(536870912, PioI2s_floatToInt32(0.25f));
    TEST_ASSERT_EQUAL_INT32(1610612736, PioI2s_floatToInt32(0.75f));
    // The largest float strictly less than 1.0 is 1 - 2^-24; multiplying it
    // by 2^31 yields the exactly representable value 2147483520.
    TEST_ASSERT_EQUAL_INT32(2147483520, PioI2s_floatToInt32(0.99999994f));
}

static void testFloatToInt32Negative(void) {
    TEST_ASSERT_EQUAL_INT32(-1073741824, PioI2s_floatToInt32(-0.5f));
    TEST_ASSERT_EQUAL_INT32(-536870912, PioI2s_floatToInt32(-0.25f));
    TEST_ASSERT_EQUAL_INT32(-1610612736, PioI2s_floatToInt32(-0.75f));
}

static void testFloatToInt32Saturates(void) {
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, PioI2s_floatToInt32(1.0f));
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, PioI2s_floatToInt32(1.5f));
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, PioI2s_floatToInt32(1000.0f));
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, PioI2s_floatToInt32(-1.0f));
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, PioI2s_floatToInt32(-1.5f));
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, PioI2s_floatToInt32(-1000.0f));
}

static void testFloatToInt32NaN(void) {
    TEST_ASSERT_EQUAL_INT32(0, PioI2s_floatToInt32(NAN));
}

static void testFloatToInt32TinyValues(void) {
    TEST_ASSERT_EQUAL_INT32(2, PioI2s_floatToInt32(0.000000001f));
    TEST_ASSERT_EQUAL_INT32(-2, PioI2s_floatToInt32(-0.000000001f));
    TEST_ASSERT_EQUAL_INT32(0, PioI2s_floatToInt32(0.0000000001f));
}

static void testWriteSamplesInterleaves(void) {
    int32_t output[6] = {0};
    PioI2s_writeSamples(0x11111111, 0x22222222, output, 0);
    TEST_ASSERT_EQUAL_INT32(0x11111111, output[0]);
    TEST_ASSERT_EQUAL_INT32(0x22222222, output[1]);

    PioI2s_writeSamples(0x33333333, 0x44444444, output, 2);
    TEST_ASSERT_EQUAL_INT32(0, output[2]);
    TEST_ASSERT_EQUAL_INT32(0, output[3]);
    TEST_ASSERT_EQUAL_INT32(0x33333333, output[4]);
    TEST_ASSERT_EQUAL_INT32(0x44444444, output[5]);
}

static void testWriteStereoConvertsAndInterleaves(void) {
    const size_t blockSize = 4;
    float left[4] = {0.25f, -0.5f, 0.75f, 0.0f};
    float right[4] = {-0.25f, 0.5f, 0.0f, -0.75f};
    int32_t output[8] = {0};
    size_t i;

    PioI2s_writeStereo(left, right, blockSize, output);

    for (i = 0; i < blockSize; i++) {
        TEST_ASSERT_EQUAL_INT32(PioI2s_floatToInt32(left[i]), output[i * 2]);
        TEST_ASSERT_EQUAL_INT32(PioI2s_floatToInt32(right[i]), output[i * 2 + 1]);
    }
}

static void testWriteMonoDuplicatesSamples(void) {
    const size_t blockSize = 4;
    float mono[4] = {0.25f, -0.5f, 0.0f, 0.75f};
    int32_t output[8] = {0};
    size_t i;

    PioI2s_writeMono(mono, blockSize, output);

    for (i = 0; i < blockSize; i++) {
        int32_t expected = PioI2s_floatToInt32(mono[i]);
        TEST_ASSERT_EQUAL_INT32(expected, output[i * 2]);
        TEST_ASSERT_EQUAL_INT32(expected, output[i * 2 + 1]);
    }
}

static inline void runConversionTests(void) {
    RUN_TEST(testFloatToInt32Zero);
    RUN_TEST(testFloatToInt32Positive);
    RUN_TEST(testFloatToInt32Negative);
    RUN_TEST(testFloatToInt32Saturates);
    RUN_TEST(testFloatToInt32NaN);
    RUN_TEST(testFloatToInt32TinyValues);
    RUN_TEST(testWriteSamplesInterleaves);
    RUN_TEST(testWriteStereoConvertsAndInterleaves);
    RUN_TEST(testWriteMonoDuplicatesSamples);
}

#endif // TEST_CONVERSION_TESTS_H
