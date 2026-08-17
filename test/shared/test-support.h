/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_TEST_SUPPORT_H
#define TEST_TEST_SUPPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/dma.h"
#include "hardware/irq.h"
#ifdef PIOI2S_TEST_DEVICE
#include "hardware/structs/dma_debug.h"
#endif
#include "pio-i2s.h"

#define TEST_DATA_PIN 9
#define TEST_BCLK_PIN 10
#define TEST_BIT_DEPTH 32
#define TEST_SAMPLE_RATE 48000
#define TEST_BLOCK_SIZE 16
#define TEST_DMA_IRQ DMA_IRQ_0
// 153600000 / (48000 * 2 * 32 * 2) = 25.0 exactly.
#define TEST_EXPECTED_CLOCK_DIV 25.0f
#define TEST_STEREO_BLOCK_SIZE (TEST_BLOCK_SIZE * PioI2S_NUM_CHANNELS)
#define TEST_DOUBLE_BUFFER_SIZE (TEST_STEREO_BLOCK_SIZE * 2)

static inline struct PioI2S_Config testDefaultConfig(void) {
    struct PioI2S_Config config = {
        .dataPin = TEST_DATA_PIN,
        .bclkPin = TEST_BCLK_PIN,
        .bitDepth = TEST_BIT_DEPTH,
        .sampleRate = TEST_SAMPLE_RATE,
        .blockSize = TEST_BLOCK_SIZE,
        .dmaIRQ = TEST_DMA_IRQ,
        .pio = pio0
    };
    return config;
}

// Set by tests before starting audio output.
extern struct PioI2S* testCurrentInstance;

extern void (*testDMAHandlerCallback)(void);

void testDMAIRQHandler(void);

// Acknowledges the interrupt without touching the output buffer.
void testAcknowledgeHandler(void);

// On the device the registers are written verbatim by pio_sm_set_config
// (the default GPIO base is 0), so reading them directly is valid.
static inline pio_sm_config testPioGetSMConfig(PIO pio, uint sm) {
    pio_sm_config config = pio_get_default_sm_config();
    config.clkdiv = pio->sm[sm].clkdiv;
    config.execctrl = pio->sm[sm].execctrl;
    config.shiftctrl = pio->sm[sm].shiftctrl;
    config.pinctrl = pio->sm[sm].pinctrl;
    return config;
}

static inline bool testPioIsSMEnabled(PIO pio, uint sm) {
    return (pio->ctrl & (1u << (PIO_CTRL_SM_ENABLE_LSB + sm))) != 0;
}

// Every test that calls PioI2S_init must track the instance. Unity aborts
// a test on its first failed assertion, and tearDown() runs after the test
// function has returned, so this stores a copy of the instance (and its
// config) in file-scope storage, which tearDown() then releases.
void testTrackInstance(struct PioI2S* self);

void testUntrackInstance(struct PioI2S* self);

void testRegisterCleanup(void (*fn)(void));

void testRunCleanups(void);

// The library has no teardown API, so this mirrors the resources claimed
// by PioI2S_init.
void testPioI2SDeinit(struct PioI2S* self);

// Disables, aborts, and fully resets a DMA channel (including its DREQ
// credit counter) so a later test can safely reuse it.
void testResetDMAChannel(uint channel);

#ifdef PIOI2S_TEST_HOST
#include <setjmp.h>

extern jmp_buf testPanicJumpBuf;
extern bool testPanicActive;
extern char testPanicMessage[128];

// Runs fn, expecting it to panic. Returns true if it did, and copies the
// panic message into message (which may be NULL). Returns false if fn
// returned normally. Host-only: the device's real panic() halts the chip.
bool testRunExpectingPanic(void (*fn)(void), char* message);
#endif

#endif // TEST_TEST_SUPPORT_H
