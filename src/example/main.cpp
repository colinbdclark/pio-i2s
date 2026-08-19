/*
Copyright 2025-6 The pio-i2s Contributors.
Licensed under the BSD-3 License.
*/

#include <stdlib.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include <pio-i2s.h>
#include "synth.h"

// The CPU clock speed in kilohertz.
// 153.6 MHz is an exact division of 25 with a 48 KHz stream.
#define CPU_CLOCK_SPEED_KHZ 153600
#define BLOCK_SIZE 48
#define DOUBLE_BUFFER_SIZE (BLOCK_SIZE * PioI2S_NUM_CHANNELS * 2)
#define GAIN 0.25f
#define FREQUENCY 120.0f

PioI2S_Config i2sConfig = {
    .dataPin = 9,
    .bclkPin = 10,
    .bitDepth = 32,
    .sampleRate = 48000,
    .blockSize = BLOCK_SIZE,
    .dmaIRQ = DMA_IRQ_0,
    .pio = pio0
};

Oscillator<BLOCK_SIZE> osc;
PioI2S i2s;
int32_t output[DOUBLE_BUFFER_SIZE] = {0};

void dmaHandler() {
    osc.generateBlock();

    int32_t* bufferToFill = PioI2S_nextOutputBuffer(&i2s);
    PioI2s_writeMono(osc.output, BLOCK_SIZE, bufferToFill);
    PioI2S_endDMAInterruptHandler(&i2s);
}

int main() {
    set_sys_clock_khz(CPU_CLOCK_SPEED_KHZ, true);

    osc.init({(float) i2sConfig.sampleRate}, WaveShapes::sine);
    osc.frequency = FREQUENCY;
    osc.gain = GAIN;

    PioI2S_init(&i2s, &i2sConfig, output, dmaHandler);
    PioI2S_start(&i2s);

    while (true) {
        tight_loop_contents();
    }
}
