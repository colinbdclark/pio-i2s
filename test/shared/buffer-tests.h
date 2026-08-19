/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_BUFFER_TESTS_H
#define TEST_BUFFER_TESTS_H

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include <pio-i2s-config.h>
#include "pio-i2s.h"
#include "test-support.h"

static void testNextOutputBufferAlternates(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE] = {0};

    self.config = &config;
    self.outputDoubleBuffer = output;
    self.stereoBlockSize = TEST_STEREO_BLOCK_SIZE;
    self.bufferPointers[0] = &output[0];
    self.bufferPointers[1] = &output[TEST_STEREO_BLOCK_SIZE];

    TEST_ASSERT_EQUAL_PTR(&output[0], PioI2S_nextOutputBuffer(&self));

    TEST_ASSERT_EQUAL_PTR(&output[TEST_STEREO_BLOCK_SIZE], PioI2S_nextOutputBuffer(&self));

    TEST_ASSERT_EQUAL_PTR(&output[0], PioI2S_nextOutputBuffer(&self));
}

#if PioI2S_ZERO_ON_UNDERRUN
static void testNextOutputBufferZeroesOnUnderrun(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE];
    int32_t* bufferToFill;
    size_t i;

    memset(output, 0xAB, sizeof(output));

    self.config = &config;
    self.outputDoubleBuffer = output;
    self.stereoBlockSize = TEST_STEREO_BLOCK_SIZE;
    self.bufferPointers[0] = &output[0];
    self.bufferPointers[1] = &output[TEST_STEREO_BLOCK_SIZE];

    bufferToFill = PioI2S_nextOutputBuffer(&self);
    TEST_ASSERT_EQUAL_PTR(&output[0], bufferToFill);

    for (i = 0; i < TEST_STEREO_BLOCK_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT32(0, bufferToFill[i]);
    }

    for (i = 0; i < TEST_STEREO_BLOCK_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT32(0xABABABAB, output[TEST_STEREO_BLOCK_SIZE + i]);
    }
}
#endif

static void testEndDMAInterruptHandlerIncrementsBlockCount(void) {
    struct PioI2S self = {0};
    self.dmaIRQIdx = 0;
    self.dataChannel = 2;
    self.numBlocksTransferred = 5;

    PioI2S_endDMAInterruptHandler(&self);
    TEST_ASSERT_EQUAL_UINT64(6, self.numBlocksTransferred);

    PioI2S_endDMAInterruptHandler(&self);
    TEST_ASSERT_EQUAL_UINT64(7, self.numBlocksTransferred);
}

static inline void runBufferTests(void) {
    RUN_TEST(testNextOutputBufferAlternates);
#if PioI2S_ZERO_ON_UNDERRUN
    RUN_TEST(testNextOutputBufferZeroesOnUnderrun);
#endif
    RUN_TEST(testEndDMAInterruptHandlerIncrementsBlockCount);
}

#endif // TEST_BUFFER_TESTS_H
