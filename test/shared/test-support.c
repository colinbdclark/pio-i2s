/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#include "test-support.h"

#ifdef PIOI2S_TEST_HOST
#include <string.h>

jmp_buf testPanicJumpBuf;
bool testPanicActive = false;
char testPanicMessage[128];

bool testRunExpectingPanic(void (*fn)(void), char* message) {
    if (setjmp(testPanicJumpBuf) == 0) {
        testPanicActive = true;
        fn();
        testPanicActive = false;
        return false;
    }
    testPanicActive = false;
    if (message != NULL) {
        strncpy(message, testPanicMessage, 127);
        message[127] = '\0';
    }
    return true;
}
#endif

struct PioI2S* testCurrentInstance = NULL;
void (*testDMAHandlerCallback)(void) = NULL;

void testDMAIRQHandler(void) {
    if (testCurrentInstance == NULL || testDMAHandlerCallback == NULL) {
        // Without an instance, there is no way to know which channel is
        // pending. Clear every channel so a stray interrupt cannot
        // re-enter this handler forever.
        for (uint ch = 0; ch < NUM_DMA_CHANNELS; ch++) {
            dma_irqn_acknowledge_channel(0, ch);
        }
        return;
    }
    testDMAHandlerCallback();
}

void testAcknowledgeHandler(void) {
    PioI2S_endDMAInterruptHandler(testCurrentInstance);
}

static struct PioI2S g_trackedInstances[4];
static struct PioI2S_Config g_trackedConfigs[4];
static uint g_numTrackedInstances = 0;
static void (*g_cleanups[4])(void);
static uint g_numCleanups = 0;

static bool testSameInstance(const struct PioI2S* a, const struct PioI2S* b) {
    return a->sm == b->sm &&
        a->dataChannel == b->dataChannel &&
        a->controlChannel == b->controlChannel;
}

void testTrackInstance(struct PioI2S* self) {
    uint i;
    for (i = 0; i < g_numTrackedInstances; i++) {
        if (testSameInstance(&g_trackedInstances[i], self)) {
            return;
        }
    }
    if (g_numTrackedInstances >= 4) {
        panic("Too many tracked PioI2S instances");
    }
    g_trackedInstances[g_numTrackedInstances] = *self;
    g_trackedConfigs[g_numTrackedInstances] = *self->config;
    g_trackedInstances[g_numTrackedInstances].config =
        &g_trackedConfigs[g_numTrackedInstances];
    g_numTrackedInstances++;
}

void testUntrackInstance(struct PioI2S* self) {
    uint i;
    for (i = 0; i < g_numTrackedInstances; i++) {
        if (testSameInstance(&g_trackedInstances[i], self)) {
            uint last = --g_numTrackedInstances;
            if (i != last) {
                g_trackedInstances[i] = g_trackedInstances[last];
                g_trackedConfigs[i] = g_trackedConfigs[last];
                g_trackedInstances[i].config = &g_trackedConfigs[i];
            }
            return;
        }
    }
}

void testRegisterCleanup(void (*fn)(void)) {
    if (g_numCleanups >= 4) {
        panic("Too many registered test cleanups");
    }
    g_cleanups[g_numCleanups++] = fn;
}

void testRunCleanups(void) {
    while (g_numTrackedInstances > 0) {
        testPioI2SDeinit(&g_trackedInstances[0]);
    }
    while (g_numCleanups > 0) {
        g_cleanups[--g_numCleanups]();
    }
    testDMAHandlerCallback = NULL;
    testCurrentInstance = NULL;
}

// Returns the channel to a state a later test can safely reuse.
// dma_channel_cleanup disables the channel and its chaining before
// aborting, disables its interrupts, and clears its completion flag.
// Aborting a DREQ-paced channel mid-transfer also leaves the channel's
// DREQ credit counter out of sync with the peripheral, which starves
// every later transfer on that channel; writing DBG_CTDREQ resets it.
void testResetDMAChannel(uint channel) {
    dma_channel_cleanup(channel);
#ifdef PIOI2S_TEST_DEVICE
    dma_debug_hw->ch[channel].dbg_ctdreq = 0;
#endif
}

void testPioI2SDeinit(struct PioI2S* self) {
    testUntrackInstance(self);

    irq_set_enabled(self->config->dmaIRQ, false);
    pio_sm_set_enabled(self->config->pio, self->sm, false);
    testResetDMAChannel((uint) self->controlChannel);
    testResetDMAChannel((uint) self->dataChannel);
    irq_remove_handler(self->config->dmaIRQ, self->dmaHandler);

    pio_remove_program(self->config->pio, &PioI2S_out_program,
        (uint) self->programOffset);
    pio_sm_unclaim(self->config->pio, self->sm);
    dma_channel_unclaim(self->dataChannel);
    dma_channel_unclaim(self->controlChannel);
}
