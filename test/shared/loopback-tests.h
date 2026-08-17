/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

// Bitstream format produced by the library's PIO program
//
// Each stereo frame produces 64 BCLK cycles. Per channel: the pull (BCLK
// low), then `set x, 30` (BCLK high), then the loop `out pins, 1` (BCLK
// low) + `jmp x--` (BCLK high). `jmp x--` falls through only when X
// reaches 0, so the loop body runs 31 times, emitting bits 31..1 MSB
// first. Bit 0 is never transmitted; the data pin holds bit 1 through the
// next channel's pull.
//
// `out` changes the data pin on BCLK falling edges, and each rising edge
// carries one bit, so sampling on the rising edges yields, per frame:
//
// [0] = bit 1 of the previous frame's right sample (held through the
// left channel's pull)
// [1..31] = bits 31..1 of the left sample, MSB first
// [32] = bit 1 of the left sample (held through the right channel's pull)
// [33..63] = bits 31..1 of the right sample, MSB first
//
// The expected bitstream encodes this explicitly, so any change to the
// program's bit count or order fails this test.

#ifndef TEST_LOOPBACK_TESTS_H
#define TEST_LOOPBACK_TESTS_H

#ifdef PIOI2S_TEST_DEVICE

#include <stdint.h>
#include <stdio.h>
#include "unity.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "capture.pio.h"
#include "test-support.h"

enum {
    kCaptureDataPin = 20,
    kCaptureBclkPin = 21,
    kCaptureWsPin = 22
};

_Static_assert(kCaptureBclkPin == 21, "capture.pio waits on GPIO 21");

// 256 words of capture: comfortably more than one full double-buffer period
// (2048 bits) plus startup slack.
enum {
    kCaptureWords = 256,
    kCaptureBits = kCaptureWords * 32,
    // The first few stereo frames are excluded from the comparison: the TX
    // FIFO is not guaranteed to be primed when the PIO pulls its very first
    // sample, so the beginning of the stream may contain a startup
    // transient.
    kFramesToSkip = 4
};

enum {
    kBitsPerFrame = 64,
    kFramesPerPeriod = TEST_BLOCK_SIZE * 2, // the double buffer
    kBitsPerPeriod = kFramesPerPeriod * kBitsPerFrame
};

typedef struct CaptureChannel {
    uint sm;
    uint programOffset;
    uint dmaChannel;
    bool active;
    uint32_t buffer[kCaptureWords];
} CaptureChannel;

// File-scope so tearDown() can release them after a failed assertion.
static CaptureChannel g_dataCapture;
static CaptureChannel g_wsCapture;

static void captureSetup(CaptureChannel* ch, uint pin) {
    // RP2350 pads reset with their input buffer disabled and pad
    // isolation latched; gpio_init clears both so the capture state
    // machine can observe the pin.
    gpio_init(pin);
    ch->sm = pio_claim_unused_sm(pio1, true);
    ch->programOffset = pio_add_program(pio1, &capture_program);

    pio_sm_config config = capture_program_get_default_config(ch->programOffset);
    sm_config_set_in_pins(&config, pin);
    sm_config_set_in_shift(&config, false, true, 32);
    sm_config_set_clkdiv(&config, 1.0f);
    pio_sm_init(pio1, ch->sm, ch->programOffset, &config);

    ch->dmaChannel = dma_claim_unused_channel(true);
    dma_channel_config dmaConfig = dma_channel_get_default_config(ch->dmaChannel);
    channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_32);
    channel_config_set_read_increment(&dmaConfig, false);
    channel_config_set_write_increment(&dmaConfig, true);
    channel_config_set_dreq(&dmaConfig, pio_get_dreq(pio1, ch->sm, false));
    dma_channel_configure(ch->dmaChannel, &dmaConfig,
        ch->buffer, &pio1->rxf[ch->sm], kCaptureWords, false);
    ch->active = true;
}

static void captureTeardown(CaptureChannel* ch) {
    if (!ch->active) {
        return;
    }
    ch->active = false;
    testResetDMAChannel(ch->dmaChannel);
    dma_channel_unclaim(ch->dmaChannel);
    pio_sm_set_enabled(pio1, ch->sm, false);
    pio_sm_unclaim(pio1, ch->sm);
    pio_remove_program(pio1, &capture_program, ch->programOffset);
}

static void loopbackCleanup(void) {
    captureTeardown(&g_wsCapture);
    captureTeardown(&g_dataCapture);
}

// Extracts captured bit j (bit 0 is the first capture).
static inline uint8_t captureBit(const uint32_t* buffer, size_t j) {
    return (uint8_t) ((buffer[j / 32] >> (31 - (j % 32))) & 1u);
}

// Builds the 64-bit sequence for one stereo frame (see the bitstream
// format above).
static void buildFrameBits(int32_t prevRight, int32_t left, int32_t right,
    uint8_t* bits) {
    int i;
    bits[0] = (uint8_t) (((uint32_t) prevRight >> 1) & 1u);
    for (i = 0; i < 31; i++) {
        bits[1 + i] = (uint8_t) (((uint32_t) left >> (31 - i)) & 1u);
    }
    bits[32] = (uint8_t) (((uint32_t) left >> 1) & 1u);
    for (i = 0; i < 31; i++) {
        bits[33 + i] = (uint8_t) (((uint32_t) right >> (31 - i)) & 1u);
    }
}

static void runLoopbackBitstreamTest(uint sampleRate) {
    // The two halves of the double buffer are filled with distinct patterns
    // so the test also verifies that the control DMA channel alternates
    // between the halves correctly.
    const int32_t leftA = 0x5A5A5A5A;
    const int32_t rightA = 0xA5A5A5A5;
    const int32_t leftB = 0x33333333;
    const int32_t rightB = 0xCCCCCCCC;

    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE] = {0};
    uint8_t expected[kBitsPerPeriod];
    size_t compareStart;
    size_t f;
    size_t j;
    bool timedOut = false;
    bool dataFailed = false;
    bool wsFailed = false;
    uint64_t start;

    for (j = 0; j < (size_t) TEST_BLOCK_SIZE; j++) {
        output[j * 2] = leftA;
        output[j * 2 + 1] = rightA;
        output[TEST_STEREO_BLOCK_SIZE + j * 2] = leftB;
        output[TEST_STEREO_BLOCK_SIZE + j * 2 + 1] = rightB;
    }

    testDMAHandlerCallback = testAcknowledgeHandler;
    testCurrentInstance = &self;
    config.sampleRate = sampleRate;
    PioI2S_init(&self, &config, output, testDMAIRQHandler);
    testTrackInstance(&self);

    // Set up the capture channels before starting I2S output, so both
    // capture state machines stall on the first BCLK rising edge together
    // and their samples stay aligned with each other.
    gpio_init(kCaptureBclkPin);
    captureSetup(&g_dataCapture, kCaptureDataPin);
    captureSetup(&g_wsCapture, kCaptureWsPin);
    testRegisterCleanup(loopbackCleanup);
    dma_channel_start(g_dataCapture.dmaChannel);
    dma_channel_start(g_wsCapture.dmaChannel);
    pio_sm_set_enabled(pio1, g_dataCapture.sm, true);
    pio_sm_set_enabled(pio1, g_wsCapture.sm, true);

    PioI2S_start(&self);

    // Wait for the capture DMAs to fill their buffers. The capture is
    // clocked by BCLK, so if the library fails to produce a bit clock this
    // times out and the test fails.
    start = time_us_64();
    while (dma_channel_is_busy(g_dataCapture.dmaChannel) ||
        dma_channel_is_busy(g_wsCapture.dmaChannel)) {
        if (time_us_64() - start > 5000000) {
            timedOut = true;
            break;
        }
    }

    testPioI2SDeinit(&self);

    if (timedOut) {
        TEST_FAIL_MESSAGE("Timed out waiting for the loopback capture "
            "(is the BCLK jumper wire connected?)");
        return;
    }

    // Each frame's first bit depends on the previous frame's right sample,
    // including across the half-A/half-B boundary.
    for (f = 0; f < kFramesPerPeriod; f++) {
        int32_t left = f < TEST_BLOCK_SIZE ? leftA : leftB;
        int32_t right = f < TEST_BLOCK_SIZE ? rightA : rightB;
        size_t prevFrame = f == 0 ? kFramesPerPeriod - 1 : f - 1;
        int32_t prevRight = prevFrame < TEST_BLOCK_SIZE ? rightA : rightB;
        buildFrameBits(prevRight, left, right, &expected[f * kBitsPerFrame]);
    }

    compareStart = kFramesToSkip * kBitsPerFrame;
    for (j = compareStart; j < kCaptureBits && !dataFailed; j++) {
        uint8_t expectedBit = expected[j % kBitsPerPeriod];
        if (captureBit(g_dataCapture.buffer, j) != expectedBit) {
            char message[96];
            snprintf(message, sizeof(message),
                "Data bit %u mismatch: expected %u, got %u",
                (unsigned) j, expectedBit, captureBit(g_dataCapture.buffer, j));
            TEST_FAIL_MESSAGE(message);
            dataFailed = true;
        }
    }

    // The word select line must frame each channel: 32 BCLK cycles low
    // (left channel) followed by 32 cycles high (right channel).
    for (j = compareStart; j < kCaptureBits && !wsFailed; j++) {
        uint8_t expectedWs = (j % kBitsPerFrame) >= 32 ? 1 : 0;
        if (captureBit(g_wsCapture.buffer, j) != expectedWs) {
            char message[96];
            snprintf(message, sizeof(message),
                "WS bit %u mismatch: expected %u, got %u",
                (unsigned) j, expectedWs, captureBit(g_wsCapture.buffer, j));
            TEST_FAIL_MESSAGE(message);
            wsFailed = true;
        }
    }
}

static void testLoopbackBitstream(void) {
    // 153600000 / (48000 * 2 * 32 * 2) = 25.0 exactly.
    set_sys_clock_khz(153600, true);
    runLoopbackBitstreamTest(TEST_SAMPLE_RATE);
}

static void testLoopbackBitstreamAtDefaultClock(void) {
    // 150000000 / (48000 * 2 * 32 * 2) = 24 + 106/256.
    set_sys_clock_khz(150000, true);
    runLoopbackBitstreamTest(TEST_SAMPLE_RATE);
}

static void testLoopbackBitstreamAtHighSampleRate(void) {
    // 153600000 / (192000 * 2 * 32 * 2) = 6 + 64/256.
    set_sys_clock_khz(153600, true);
    runLoopbackBitstreamTest(192000);
}

static void testLoopbackBitstreamAtHighClock(void) {
    // 300000000 / (48000 * 2 * 32 * 2) = 48 + 212/256.
    set_sys_clock_khz(300000, true);
    runLoopbackBitstreamTest(TEST_SAMPLE_RATE);
}

static inline void runLoopbackTests(void) {
    RUN_TEST(testLoopbackBitstream);
    RUN_TEST(testLoopbackBitstreamAtDefaultClock);
    RUN_TEST(testLoopbackBitstreamAtHighSampleRate);
    RUN_TEST(testLoopbackBitstreamAtHighClock);
}

#endif // PIOI2S_TEST_DEVICE
#endif // TEST_LOOPBACK_TESTS_H
