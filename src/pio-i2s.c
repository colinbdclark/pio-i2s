/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#include <math.h>
#include <string.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pio-i2s-config.h>
#include <pio-i2s.h>

float PioI2S_calculateClockDivision(struct PioI2S* self) {
    float clockSpeed = (float) clock_get_hz(clk_sys);
    float bclkSpeed = (float) self->config->sampleRate *
        (float) PioI2S_NUM_CHANNELS * (float) self->config->bitDepth;
    float pioSpeed = bclkSpeed * (float) PioI2S_PIO_INSTRUCTIONS_PER_BIT;
    float clockDivision = clockSpeed / pioSpeed;

    return clockDivision;
}

void PicoI2S_verifyPIOClockDivision(float frequencyRatio) {
    if (frequencyRatio < 1.0f) {
        panic("PIO clock ratio %f is faster than the system clock.",
            frequencyRatio);
    }

    float integral;
    float fractional = modff(frequencyRatio, &integral);

    // The library supports clock ratios less than 256.
    if (integral > 255.0f) {
        panic("PIO clock ratio %f is too large.",
            frequencyRatio);
    }

    // The PIO divider stores 8 fractional bits, so a ratio is
    // representable only if its fractional part is a multiple of 1/256.
    float scaled = fractional * 256.0f;
    float scaledIntegral;
    float remainder = modff(scaled, &scaledIntegral);

    if (remainder > 0.0f) {
        panic("PIO clock ratio %f cannot be represented accurately.",
            frequencyRatio);
    }
}

void PioI2s_initPIOProgram(struct PioI2S* self) {
    self->programOffset =
        pio_add_program(self->config->pio, &PioI2S_out_program);
    pio_sm_config sm_config = PioI2S_out_program_get_default_config(
        self->programOffset);

    pio_gpio_init(self->config->pio, self->config->dataPin);
    pio_gpio_init(self->config->pio, self->config->bclkPin);
    pio_gpio_init(self->config->pio, self->config->bclkPin + 1);
    sm_config_set_out_pins(&sm_config, self->config->dataPin, 1);
    sm_config_set_sideset_pin_base(&sm_config, self->config->bclkPin);
    sm_config_set_out_shift(&sm_config, false, false, self->config->bitDepth);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);

    float clockDivision = PioI2S_calculateClockDivision(self);
    PicoI2S_verifyPIOClockDivision(clockDivision);
    sm_config_set_clkdiv(&sm_config, clockDivision);
    pio_sm_init(self->config->pio, self->sm, self->programOffset, &sm_config);

    uint32_t pinMask = (1u << self->config->dataPin) |
        (3u << self->config->bclkPin);
    pio_sm_set_pindirs_with_mask(self->config->pio, self->sm, pinMask, pinMask);
    pio_sm_set_pins(self->config->pio, self->sm, 0);
}

void PioI2S_initControlChannel(struct PioI2S* self) {
    dma_channel_config config = dma_channel_get_default_config(
        self->controlChannel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_ring(&config, false, 3);

    dma_channel_configure(self->controlChannel, &config,
        &dma_hw->ch[self->dataChannel].al3_read_addr_trig,
        &self->bufferPointers, 1, false);
}

void PioI2S_initDataChannel(struct PioI2S* self) {
    dma_channel_config config = dma_channel_get_default_config(
        self->dataChannel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, pio_get_dreq(
        self->config->pio, self->sm, true));
    channel_config_set_chain_to(&config, self->controlChannel);

    dma_channel_configure(self->dataChannel, &config,
        &self->config->pio->txf[self->sm],
        NULL, // The read address is set by the control channel.
        self->stereoBlockSize, false);
}

void PioI2S_initDMAIRQ(struct PioI2S* self) {
    // A newly claimed channel can carry a stale completion flag from earlier use.
    // Clear it so enabling the IRQ cannot fire spuriously.
    dma_irqn_acknowledge_channel(self->dmaIRQIdx, self->dataChannel);
    dma_irqn_set_channel_enabled(self->dmaIRQIdx, self->dataChannel, true);
    irq_set_exclusive_handler(self->config->dmaIRQ, self->dmaHandler);
    irq_set_enabled(self->config->dmaIRQ, true);
}

void PioI2S_initDMA(struct PioI2S* self) {
    PioI2S_initDataChannel(self);
    PioI2S_initControlChannel(self);
    PioI2S_initDMAIRQ(self);
}

void PioI2S_init(struct PioI2S* self, struct PioI2S_Config* config,
    int32_t* outputDoubleBuffer, void(*dmaHandler)(void)) {
    self->config = config;
    self->sm = pio_claim_unused_sm(self->config->pio, true);
    self->dataChannel = dma_claim_unused_channel(true);
    self->controlChannel = dma_claim_unused_channel(true);
    self->stereoBlockSize = self->config->blockSize * PioI2S_NUM_CHANNELS;
    self->doubleBufferSize = self->stereoBlockSize * 2;
    self->startTime = 0;
    self->numBlocksTransferred = 0;
    self->outputDoubleBuffer = outputDoubleBuffer;
    self->bufferPointers[0] = &self->outputDoubleBuffer[0];
    self->bufferPointers[1] = &self->outputDoubleBuffer[self->stereoBlockSize];
    self->bufferPointerIdx = 0;
    self->dmaHandler = dmaHandler;

    if (self->config->dmaIRQ != DMA_IRQ_0 && self->config->dmaIRQ != DMA_IRQ_1) {
        panic("Invalid DMA IRQ %u.", self->config->dmaIRQ);
    }
    self->dmaIRQIdx = self->config->dmaIRQ - DMA_IRQ_0;

    PioI2s_initPIOProgram(self);
    PioI2S_initDMA(self);
}

void PioI2S_start(struct PioI2S* self) {
    self->startTime = time_us_64();
    dma_channel_start(self->controlChannel);
    pio_sm_set_enabled(self->config->pio, self->sm, true);
}

inline int32_t* PioI2S_nextOutputBuffer(struct PioI2S* self) {
    int32_t* bufferToFill = self->bufferPointers[self->bufferPointerIdx];
    self->bufferPointerIdx = 1 - self->bufferPointerIdx;
#if PioI2S_ZERO_ON_UNDERRUN
    memset(bufferToFill, 0, self->stereoBlockSize * sizeof(*bufferToFill));
#endif
    return bufferToFill;
}

inline void PioI2S_endDMAInterruptHandler(struct PioI2S* self) {
    self->numBlocksTransferred++;
    dma_irqn_acknowledge_channel(self->dmaIRQIdx, self->dataChannel);
}

inline int32_t PioI2s_floatToInt32(float sample) {
#if defined(__arm__)
    // The ARM conversion instruction rounds to zero, saturates out-of-range values,
    // and maps NaN to 0.
    // This cast is well defined for any input on ARM targets, but could be UB elsewhere.
    return (int32_t) (sample * 2147483647.0f);
#else
    if (isnan(sample)) {
        return 0;
    }
    if (sample >= 1.0f) {
        return INT32_MAX;
    }
    if (sample <= -1.0f) {
        return INT32_MIN;
    }
    return (int32_t) (sample * 2147483647.0f);
#endif
}

inline void PioI2s_writeSamples(int32_t left, int32_t right,
    int32_t* interleavedOutput, size_t i) {
    interleavedOutput[i * 2] = left;
    interleavedOutput[i * 2 + 1] = right;
}

inline void PioI2s_writeStereo(float* left, float* right,
    size_t blockSize, int32_t* interleavedOutput) {
    for (size_t i = 0; i < blockSize; i++) {
        float leftSample = left[i];
        int32_t leftConverted = PioI2s_floatToInt32(leftSample);
        float rightSample = right[i];
        int32_t rightConverted = PioI2s_floatToInt32(rightSample);
        PioI2s_writeSamples(leftConverted, rightConverted,
            interleavedOutput, i);
    }
}

inline void PioI2s_writeMono(float* mono, size_t blockSize,
    int32_t* interleavedOutput) {
    for (size_t i = 0; i < blockSize; i++) {
        int32_t converted = PioI2s_floatToInt32(mono[i]);
        PioI2s_writeSamples(converted, converted, interleavedOutput, i);
    }
}
