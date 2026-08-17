/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_HARDWARE_TESTS_H
#define TEST_HARDWARE_TESTS_H

#ifdef PIOI2S_TEST_DEVICE

#include <stdint.h>
#include "unity.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "test-support.h"

static volatile uint32_t g_irqCount;

static void countIRQHandler(void) {
    g_irqCount = g_irqCount + 1;
    PioI2S_endDMAInterruptHandler(testCurrentInstance);
}

static void runDMAInterruptPerBlockTest(uint sampleRate) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE] = {0};
    const uint32_t blocksToWait = 300;
    const uint64_t expectedDuration =
        (uint64_t) blocksToWait * TEST_BLOCK_SIZE * 1000000ull / sampleRate;
    bool timedOut = false;
    uint64_t start;
    uint64_t elapsed;

    g_irqCount = 0;
    testDMAHandlerCallback = countIRQHandler;
    testCurrentInstance = &self;
    config.sampleRate = sampleRate;
    PioI2S_init(&self, &config, output, testDMAIRQHandler);
    testTrackInstance(&self);

    start = time_us_64();
    PioI2S_start(&self);

    // g_irqCount is volatile, so spinning on it reloads it each iteration
    // even though the IRQ mutates it asynchronously.
    while (g_irqCount < blocksToWait) {
        if (time_us_64() - start > expectedDuration * 2 + 100000) {
            timedOut = true;
            break;
        }
    }
    elapsed = time_us_64() - start;

    // Stop the output before asserting so the counters stop changing.
    testPioI2SDeinit(&self);

    if (timedOut) {
        TEST_FAIL_MESSAGE("Timed out waiting for DMA block interrupts");
    }

    // The block count can overshoot by a few blocks while the loop exits.
    TEST_ASSERT_TRUE(self.numBlocksTransferred >= blocksToWait);
    TEST_ASSERT_TRUE(self.numBlocksTransferred <= blocksToWait + 3);

    TEST_ASSERT_TRUE(g_irqCount >= blocksToWait);
    TEST_ASSERT_TRUE(g_irqCount <= blocksToWait + 3);

    // Allow +/- 5% for measurement and interrupt latency.
    TEST_ASSERT_INT64_WITHIN((int64_t) (expectedDuration / 20),
        (int64_t) expectedDuration, (int64_t) elapsed);
}

static void testDMAInterruptFiresPerBlock(void) {
    // 153600000 / (48000 * 2 * 32 * 2) = 25.0 exactly.
    set_sys_clock_khz(153600, true);
    runDMAInterruptPerBlockTest(TEST_SAMPLE_RATE);
}

static void testDMAInterruptFiresPerBlockAtDefaultClock(void) {
    // 150000000 / (48000 * 2 * 32 * 2) = 24 + 106/256.
    set_sys_clock_khz(150000, true);
    runDMAInterruptPerBlockTest(TEST_SAMPLE_RATE);
}

static void testDMAInterruptFiresPerBlockAtHighSampleRate(void) {
    // 153600000 / (192000 * 2 * 32 * 2) = 6 + 64/256.
    set_sys_clock_khz(153600, true);
    runDMAInterruptPerBlockTest(192000);
}

static void testDMAInterruptFiresPerBlockAtHighClock(void) {
    // 300000000 / (48000 * 2 * 32 * 2) = 48 + 212/256.
    set_sys_clock_khz(300000, true);
    runDMAInterruptPerBlockTest(TEST_SAMPLE_RATE);
}

static inline void runHardwareTests(void) {
    RUN_TEST(testDMAInterruptFiresPerBlock);
    RUN_TEST(testDMAInterruptFiresPerBlockAtDefaultClock);
    RUN_TEST(testDMAInterruptFiresPerBlockAtHighSampleRate);
    RUN_TEST(testDMAInterruptFiresPerBlockAtHighClock);
}

#endif // PIOI2S_TEST_DEVICE
#endif // TEST_HARDWARE_TESTS_H
