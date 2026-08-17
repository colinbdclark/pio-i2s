/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_INIT_TESTS_H
#define TEST_INIT_TESTS_H

#include <stdint.h>
#include "unity.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "pio-i2s.h"
#include "test-support.h"

static void testInitConfiguresInstanceFields(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE] = {0};

    PioI2S_init(&self, &config, output, testDMAIRQHandler);
    testTrackInstance(&self);

    TEST_ASSERT_EQUAL_PTR(&config, self.config);
    TEST_ASSERT_EQUAL_INT(TEST_STEREO_BLOCK_SIZE, self.stereoBlockSize);
    TEST_ASSERT_EQUAL_INT(TEST_DOUBLE_BUFFER_SIZE, self.doubleBufferSize);
    TEST_ASSERT_EQUAL_PTR(&output[0], self.bufferPointers[0]);
    TEST_ASSERT_EQUAL_PTR(&output[TEST_STEREO_BLOCK_SIZE], self.bufferPointers[1]);
    TEST_ASSERT_EQUAL_INT(0, self.bufferPointerIdx);
    TEST_ASSERT_EQUAL_UINT64(0, self.startTime);
    TEST_ASSERT_EQUAL_UINT64(0, self.numBlocksTransferred);

    TEST_ASSERT_TRUE(self.sm >= 0 && self.sm < 4);
    TEST_ASSERT_TRUE((uint) self.dataChannel < NUM_DMA_CHANNELS);
    TEST_ASSERT_TRUE((uint) self.controlChannel < NUM_DMA_CHANNELS);
    TEST_ASSERT_TRUE(self.dataChannel != self.controlChannel);

    TEST_ASSERT_EQUAL_INT(TEST_DMA_IRQ - DMA_IRQ_0, self.dmaIRQIdx);
}

static void testInitConfiguresPIOStateMachine(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE] = {0};
    uint offset;
    pio_sm_config expected;
    pio_sm_config actual;

    PioI2S_init(&self, &config, output, testDMAIRQHandler);
    testTrackInstance(&self);

    // Rebuild the configuration the library should have written, using the
    // same SDK configuration API.
    offset = (uint) self.programOffset;
    expected = pio_get_default_sm_config();
    sm_config_set_wrap(&expected,
        offset + PioI2S_out_wrap_target, offset + PioI2S_out_wrap);
    sm_config_set_sideset(&expected, 2, false, false);
    sm_config_set_out_pins(&expected, TEST_DATA_PIN, 1);
    sm_config_set_sideset_pin_base(&expected, TEST_BCLK_PIN);
    sm_config_set_out_shift(&expected, false, false, TEST_BIT_DEPTH);
    sm_config_set_fifo_join(&expected, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&expected, TEST_EXPECTED_CLOCK_DIV);

    actual = testPioGetSMConfig(config.pio, self.sm);
    TEST_ASSERT_EQUAL_UINT32(expected.clkdiv, actual.clkdiv);
    TEST_ASSERT_EQUAL_UINT32(expected.execctrl, actual.execctrl);
    TEST_ASSERT_EQUAL_UINT32(expected.shiftctrl, actual.shiftctrl);
    TEST_ASSERT_EQUAL_UINT32(expected.pinctrl, actual.pinctrl);

    TEST_ASSERT_EQUAL_INT(offset + PioI2S_out_offset_entry_point,
        pio_sm_get_pc(config.pio, self.sm));
}

static void testInitConfiguresDMAChannels(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE] = {0};
    dma_channel_hw_t* data;
    dma_channel_hw_t* control;
    uint32_t dataCtrl;
    uint32_t controlCtrl;

    PioI2S_init(&self, &config, output, testDMAIRQHandler);
    testTrackInstance(&self);

    data = dma_channel_hw_addr(self.dataChannel);
    control = dma_channel_hw_addr(self.controlChannel);

    // The data channel writes to the PIO's TX FIFO at a fixed address, with
    // the read address set later by the control channel, transferring one
    // stereo block.
    TEST_ASSERT_EQUAL_UINT32((uint32_t) (uintptr_t) &self.config->pio->txf[self.sm],
        data->write_addr);
    TEST_ASSERT_EQUAL_UINT32(0, data->read_addr);
    // The live TRANS_COUNT register only reflects the configured count
    // once the channel is triggered; the reload value is readable in the
    // DBG_TCR debug register.
    TEST_ASSERT_EQUAL_UINT32(self.stereoBlockSize,
        dma_debug_hw->ch[self.dataChannel].dbg_tcr);

    dataCtrl = data->al1_ctrl;
    TEST_ASSERT_TRUE(dataCtrl & DMA_CH0_CTRL_TRIG_EN_BITS);
    TEST_ASSERT_TRUE(dataCtrl & DMA_CH0_CTRL_TRIG_INCR_READ_BITS);
    TEST_ASSERT_FALSE(dataCtrl & DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS);
    TEST_ASSERT_EQUAL_UINT32(DMA_SIZE_32,
        (dataCtrl & DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS) >>
            DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);
    // Paced by the PIO's TX FIFO, and chained to the control channel when
    // the block completes.
    TEST_ASSERT_EQUAL_UINT32(pio_get_dreq(config.pio, self.sm, true),
        (dataCtrl & DMA_CH0_CTRL_TRIG_TREQ_SEL_BITS) >>
            DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB);
    TEST_ASSERT_EQUAL_UINT32(self.controlChannel,
        (dataCtrl & DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS) >>
            DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB);
    // The IRQ must not be quieted: the library relies on the data channel's
    // interrupt firing at the end of each block.
    TEST_ASSERT_FALSE(dataCtrl & DMA_CH0_CTRL_TRIG_IRQ_QUIET_BITS);

    // The control channel reads the two buffer pointers cyclically (a
    // 2-entry ring of 32-bit pointers) and writes each one into the data
    // channel's read-address-trigger register, which starts the next block
    // transfer.
    TEST_ASSERT_EQUAL_UINT32((uint32_t) (uintptr_t) &self.bufferPointers,
        control->read_addr);
    TEST_ASSERT_EQUAL_UINT32(
        (uint32_t) (uintptr_t) &dma_hw->ch[self.dataChannel].al3_read_addr_trig,
        control->write_addr);
    TEST_ASSERT_EQUAL_UINT32(1,
        dma_debug_hw->ch[self.controlChannel].dbg_tcr);

    controlCtrl = control->al1_ctrl;
    TEST_ASSERT_TRUE(controlCtrl & DMA_CH0_CTRL_TRIG_EN_BITS);
    TEST_ASSERT_TRUE(controlCtrl & DMA_CH0_CTRL_TRIG_INCR_READ_BITS);
    TEST_ASSERT_FALSE(controlCtrl & DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS);
    TEST_ASSERT_EQUAL_UINT32(DMA_SIZE_32,
        (controlCtrl & DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS) >>
            DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);
    // Unpaced: the control channel transfers one pointer at full speed
    // whenever the data channel chains to it.
    TEST_ASSERT_EQUAL_UINT32(DREQ_FORCE,
        (controlCtrl & DMA_CH0_CTRL_TRIG_TREQ_SEL_BITS) >>
            DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB);
    // The ring applies to the read address (write = false) and wraps after
    // 2 entries (1 << 3 = 8 bytes). The bufferPointers array must be
    // 8-byte aligned for the ring to wrap correctly.
    TEST_ASSERT_EQUAL_UINT32(3,
        (controlCtrl & DMA_CH0_CTRL_TRIG_RING_SIZE_BITS) >>
            DMA_CH0_CTRL_TRIG_RING_SIZE_LSB);
    TEST_ASSERT_FALSE(controlCtrl & DMA_CH0_CTRL_TRIG_RING_SEL_BITS);
    // The control channel chains to itself, which the hardware treats as
    // no chaining.
    TEST_ASSERT_EQUAL_UINT32(self.controlChannel,
        (controlCtrl & DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS) >>
            DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB);
    TEST_ASSERT_EQUAL_INT(0, (int) ((uintptr_t) self.bufferPointers & 7u));
}

static void testStartEnablesOutput(void) {
    struct PioI2S self = {0};
    struct PioI2S_Config config = testDefaultConfig();
    int32_t output[TEST_DOUBLE_BUFFER_SIZE] = {0};
    uint64_t before;
    uint64_t after;
    bool wasEnabled;

    testDMAHandlerCallback = testAcknowledgeHandler;
    testCurrentInstance = &self;
    PioI2S_init(&self, &config, output, testDMAIRQHandler);
    testTrackInstance(&self);

    before = time_us_64();
    PioI2S_start(&self);
    after = time_us_64();

    // Stop the output before asserting, so no DMA interrupt can reference
    // this test's stack frame after it returns or longjmps.
    wasEnabled = testPioIsSMEnabled(config.pio, self.sm);
    testPioI2SDeinit(&self);

    TEST_ASSERT_TRUE(self.startTime >= before && self.startTime <= after);
    TEST_ASSERT_TRUE(wasEnabled);
}

static inline void runInitTests(void) {
    RUN_TEST(testInitConfiguresInstanceFields);
    RUN_TEST(testInitConfiguresPIOStateMachine);
    RUN_TEST(testInitConfiguresDMAChannels);
    RUN_TEST(testStartEnablesOutput);
}

#endif // TEST_INIT_TESTS_H
