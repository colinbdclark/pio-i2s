/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "test-support.h"

void panic(const char* fmt, ...) {
    char message[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt ? fmt : "", args);
    va_end(args);

    if (testPanicActive) {
        strncpy(testPanicMessage, message, sizeof(testPanicMessage) - 1);
        testPanicMessage[sizeof(testPanicMessage) - 1] = '\0';
        longjmp(testPanicJumpBuf, 1);
    }

    fprintf(stderr, "PANIC: %s\n", message);
    exit(1);
}

static uint64_t mockTime = 0;

uint64_t time_us_64(void) {
    return mockTime++;
}

static uint32_t mockSysClockSpeed = 153600000;

uint32_t clock_get_hz(enum clock_index clock_index) {
    return clock_index == clk_sys ? mockSysClockSpeed : 0;
}

void mockClockSetSysSpeed(uint32_t frequency) {
    mockSysClockSpeed = frequency;
}

void irq_set_exclusive_handler(uint num, irq_handler_t handler) {
    (void) num;
    (void) handler;
}

void irq_set_enabled(uint num, bool enabled) {
    (void) num;
    (void) enabled;
}

void irq_remove_handler(uint num, irq_handler_t handler) {
    (void) num;
    (void) handler;
}

pio_hw_t pio0_hw;
pio_hw_t pio1_hw;
pio_hw_t pio2_hw;

typedef struct MockPIOState {
    bool smClaimed[NUM_PIO_STATE_MACHINES];
    uint numPrograms;
    int nextProgramOffset;
} MockPIOState;

static MockPIOState mockPIOStates[3];

static MockPIOState* mockPIOGetState(PIO pio) {
    if (pio == &pio0_hw) {
        return &mockPIOStates[0];
    }
    if (pio == &pio1_hw) {
        return &mockPIOStates[1];
    }
    return &mockPIOStates[2];
}

int pio_add_program(PIO pio, const pio_program_t* program) {
    MockPIOState *state = mockPIOGetState(pio);
    // The real SDK allocates instruction memory top-down.
    state->nextProgramOffset -= program->length;
    state->numPrograms++;
    return state->nextProgramOffset;
}

void pio_remove_program(PIO pio, const pio_program_t* program, uint loaded_offset) {
    MockPIOState *state = mockPIOGetState(pio);
    (void) loaded_offset;
    if (state->numPrograms > 0) {
        state->numPrograms--;
        state->nextProgramOffset += program->length;
    }
}

void pio_gpio_init(PIO pio, uint pin) {
    (void) pio;
    (void) pin;
}

int pio_sm_init(PIO pio, uint sm, uint initial_pc, const pio_sm_config *config) {
    pio->sm[sm].clkdiv = config->clkdiv;
    pio->sm[sm].execctrl = config->execctrl;
    pio->sm[sm].shiftctrl = config->shiftctrl;
    pio->sm[sm].pinctrl = config->pinctrl;
    pio->sm[sm].addr = initial_pc;
    return 0;
}

void pio_sm_set_pindirs_with_mask(PIO pio, uint sm, uint32_t pindirs, uint32_t pin_mask) {
    (void) pio;
    (void) sm;
    (void) pindirs;
    (void) pin_mask;
}

void pio_sm_set_pins(PIO pio, uint sm, uint32_t pins) {
    (void) pio;
    (void) sm;
    (void) pins;
}

uint pio_get_dreq(PIO pio, uint sm, bool is_tx) {
    // DREQ_PIO0_TX0 = 0, DREQ_PIO0_RX0 = 4, then +8 per PIO.
    uint pioIndex = pio == &pio0_hw ? 0 : pio == &pio1_hw ? 1 : 2;
    return pioIndex * 8u + (is_tx ? 0u : 4u) + sm;
}

int pio_claim_unused_sm(PIO pio, bool required) {
    MockPIOState *state = mockPIOGetState(pio);
    for (uint sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++) {
        if (!state->smClaimed[sm]) {
            state->smClaimed[sm] = true;
            return (int) sm;
        }
    }
    if (required) {
        panic("No PIO state machines are available");
    }
    return -1;
}

void pio_sm_unclaim(PIO pio, uint sm) {
    MockPIOState *state = mockPIOGetState(pio);
    state->smClaimed[sm] = false;
}

void pio_sm_set_enabled(PIO pio, uint sm, bool enabled) {
    if (enabled) {
        pio->ctrl |= 1u << (PIO_CTRL_SM_ENABLE_LSB + sm);
    } else {
        pio->ctrl &= ~(1u << (PIO_CTRL_SM_ENABLE_LSB + sm));
    }
}

dma_hw_t dma_hw_instance;
dma_debug_hw_t dma_debug_hw_instance;

static bool mockDMAChannelClaimed[NUM_DMA_CHANNELS];

void dma_channel_configure(uint channel, const dma_channel_config *config,
    volatile void* write_addr, const volatile void* read_addr,
    uint transfer_count, bool trigger) {
    dma_channel_hw_t *hw = dma_channel_hw_addr(channel);
    hw->read_addr = (uintptr_t) read_addr;
    hw->write_addr = (uintptr_t) write_addr;
    hw->transfer_count = transfer_count;
    dma_debug_hw->ch[channel].dbg_tcr = transfer_count;
    if (trigger) {
        hw->ctrl_trig = config->ctrl | DMA_CH0_CTRL_TRIG_EN_BITS;
    } else {
        hw->al1_ctrl = config->ctrl;
    }
}

void dma_channel_cleanup(uint channel) {
    (void) channel;
}

void dma_channel_start(uint channel) {
    (void) channel;
}

int dma_claim_unused_channel(bool required) {
    for (uint channel = 0; channel < NUM_DMA_CHANNELS; channel++) {
        if (!mockDMAChannelClaimed[channel]) {
            mockDMAChannelClaimed[channel] = true;
            return (int) channel;
        }
    }
    if (required) {
        panic("No DMA channels are available");
    }
    return -1;
}

void dma_channel_unclaim(uint channel) {
    mockDMAChannelClaimed[channel] = false;
}

void dma_irqn_set_channel_enabled(uint irq_index, uint channel, bool enabled) {
    (void) irq_index;
    (void) channel;
    (void) enabled;
}

void dma_irqn_acknowledge_channel(uint irq_index, uint channel) {
    (void) irq_index;
    (void) channel;
}

void mockReset(void) {
    mockTime = 0;
    mockSysClockSpeed = 153600000;

    for (uint i = 0; i < 3; i++) {
        MockPIOState *state = &mockPIOStates[i];
        state->numPrograms = 0;
        state->nextProgramOffset = PIO_INSTRUCTION_COUNT;
        for (uint sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++) {
            state->smClaimed[sm] = false;
        }
    }

    for (uint ch = 0; ch < NUM_DMA_CHANNELS; ch++) {
        mockDMAChannelClaimed[ch] = false;
        dma_debug_hw->ch[ch].dbg_ctdreq = 0;
        dma_debug_hw->ch[ch].dbg_tcr = 0;
    }
}
