/*
* Copyright 2025-6 The pio-i2s Contributors.
* Portions derived from the Pico SDK (BSD-3-Clause, Copyright (c) 2024 Raspberry Pi Ltd.)
* Licensed under the BSD-3 License.
*/

#ifndef _MOCK_HARDWARE_PIO_H
#define _MOCK_HARDWARE_PIO_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/address_mapped.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _u
#define _u(x) x ## u
#endif

// The mock models the PIO register layout shared by the RP2040 and
// RP2350 for the fields the library and tests use: no GPIO base support,
// four 32-bit configuration words per state machine.
#define PICO_PIO_VERSION 0

#define PIO_INSTRUCTION_COUNT 32u
#define NUM_PIO_STATE_MACHINES 4u

#define PIO_SM0_CLKDIV_INT_LSB   _u(16)
#define PIO_SM0_CLKDIV_FRAC_LSB  _u(8)

#define PIO_SM0_EXECCTRL_SIDE_EN_BITS      _u(0x40000000)
#define PIO_SM0_EXECCTRL_SIDE_EN_LSB       _u(30)
#define PIO_SM0_EXECCTRL_SIDE_PINDIR_BITS  _u(0x20000000)
#define PIO_SM0_EXECCTRL_SIDE_PINDIR_LSB   _u(29)
#define PIO_SM0_EXECCTRL_WRAP_TOP_BITS     _u(0x0001f000)
#define PIO_SM0_EXECCTRL_WRAP_TOP_LSB      _u(12)
#define PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS  _u(0x00000f80)
#define PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB   _u(7)

#define PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS       _u(0x80000000)
#define PIO_SM0_SHIFTCTRL_FJOIN_RX_LSB        _u(31)
#define PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS       _u(0x40000000)
#define PIO_SM0_SHIFTCTRL_FJOIN_TX_LSB        _u(30)
#define PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS    _u(0x3e000000)
#define PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB     _u(25)
#define PIO_SM0_SHIFTCTRL_PUSH_THRESH_BITS    _u(0x01f00000)
#define PIO_SM0_SHIFTCTRL_PUSH_THRESH_LSB     _u(20)
#define PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS   _u(0x00080000)
#define PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_LSB    _u(19)
#define PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS    _u(0x00040000)
#define PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_LSB     _u(18)
#define PIO_SM0_SHIFTCTRL_AUTOPULL_BITS       _u(0x00020000)
#define PIO_SM0_SHIFTCTRL_AUTOPULL_LSB        _u(17)
#define PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS       _u(0x00010000)
#define PIO_SM0_SHIFTCTRL_AUTOPUSH_LSB        _u(16)

#define PIO_SM0_PINCTRL_SIDESET_COUNT_BITS  _u(0xe0000000)
#define PIO_SM0_PINCTRL_SIDESET_COUNT_LSB   _u(29)
#define PIO_SM0_PINCTRL_OUT_COUNT_BITS      _u(0x03f00000)
#define PIO_SM0_PINCTRL_OUT_COUNT_LSB       _u(20)
#define PIO_SM0_PINCTRL_SIDESET_BASE_BITS   _u(0x00007c00)
#define PIO_SM0_PINCTRL_SIDESET_BASE_LSB    _u(10)
#define PIO_SM0_PINCTRL_OUT_BASE_BITS       _u(0x0000001f)
#define PIO_SM0_PINCTRL_OUT_BASE_LSB        _u(0)

#define PIO_CTRL_SM_ENABLE_BITS  _u(0x0000000f)
#define PIO_CTRL_SM_ENABLE_LSB   _u(0)

typedef struct pio_program {
    const uint16_t* instructions;
    uint8_t length;
    int8_t origin; // required instruction memory origin or -1
    uint8_t pio_version;
} pio_program_t;

typedef struct pio_sm_config {
    uint32_t clkdiv;
    uint32_t execctrl;
    uint32_t shiftctrl;
    uint32_t pinctrl;
} pio_sm_config;

enum pio_fifo_join {
    PIO_FIFO_JOIN_NONE = 0,
    PIO_FIFO_JOIN_TX = 1,
    PIO_FIFO_JOIN_RX = 2
};

// Mirrors the subset of the pio_sm_hw_t registers used by the
// library and tests.
typedef struct pio_sm_hw {
    io_rw_32 clkdiv;
    io_rw_32 execctrl;
    io_rw_32 shiftctrl;
    io_rw_32 pinctrl;
    io_rw_32 addr;
} pio_sm_hw_t;

// Mirrors the subset of pio_hw_t used by the library and tests.
typedef struct pio_hw {
    io_rw_32 ctrl;
    io_rw_32 txf[NUM_PIO_STATE_MACHINES];
    pio_sm_hw_t sm[NUM_PIO_STATE_MACHINES];
} pio_hw_t;

typedef pio_hw_t *PIO;

extern pio_hw_t pio0_hw;
extern pio_hw_t pio1_hw;
extern pio_hw_t pio2_hw;

#define pio0 (&pio0_hw)
#define pio1 (&pio1_hw)
#define pio2 (&pio2_hw)

static inline void sm_config_set_clkdiv_int_frac8(pio_sm_config *c, uint32_t div_int, uint8_t div_frac8) {
    c->clkdiv =
            (((uint)div_frac8) << PIO_SM0_CLKDIV_FRAC_LSB) |
            (((uint)div_int) << PIO_SM0_CLKDIV_INT_LSB);
}

static inline void sm_config_set_clkdiv(pio_sm_config *c, float div) {
    div += 0.5f / 256.0f;
    uint32_t div_int = (uint16_t) div;
    uint8_t div_frac8 = div_int == 0 ?
        0 : (uint8_t) ((div - (float) div_int) * 256.0f);
    sm_config_set_clkdiv_int_frac8(c, div_int, div_frac8);
}

static inline void sm_config_set_wrap(pio_sm_config *c, uint wrap_target, uint wrap) {
    c->execctrl =
        (c->execctrl & ~(PIO_SM0_EXECCTRL_WRAP_TOP_BITS | PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS)) |
        (wrap_target << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB) |
        (wrap << PIO_SM0_EXECCTRL_WRAP_TOP_LSB);
}

static inline void sm_config_set_sideset(pio_sm_config *c, uint bit_count, bool optional, bool pindirs) {
    c->pinctrl =
        (c->pinctrl & ~PIO_SM0_PINCTRL_SIDESET_COUNT_BITS) |
        (bit_count << PIO_SM0_PINCTRL_SIDESET_COUNT_LSB);
    c->execctrl =
        (c->execctrl & ~(PIO_SM0_EXECCTRL_SIDE_EN_BITS | PIO_SM0_EXECCTRL_SIDE_PINDIR_BITS)) |
        ((uint32_t) optional << PIO_SM0_EXECCTRL_SIDE_EN_LSB) |
        ((uint32_t) pindirs << PIO_SM0_EXECCTRL_SIDE_PINDIR_LSB);
}

static inline void sm_config_set_out_pin_base(pio_sm_config *c, uint out_base) {
    c->pinctrl =
        (c->pinctrl & ~PIO_SM0_PINCTRL_OUT_BASE_BITS) |
        ((out_base & 31) << PIO_SM0_PINCTRL_OUT_BASE_LSB);
}

static inline void sm_config_set_out_pin_count(pio_sm_config *c, uint out_count) {
    c->pinctrl =
        (c->pinctrl & ~PIO_SM0_PINCTRL_OUT_COUNT_BITS) |
        (out_count << PIO_SM0_PINCTRL_OUT_COUNT_LSB);
}

static inline void sm_config_set_out_pins(pio_sm_config *c, uint out_base, uint out_count) {
    sm_config_set_out_pin_base(c, out_base);
    sm_config_set_out_pin_count(c, out_count);
}

static inline void sm_config_set_sideset_pin_base(pio_sm_config *c, uint sideset_base) {
    c->pinctrl =
        (c->pinctrl & ~PIO_SM0_PINCTRL_SIDESET_BASE_BITS) |
        ((sideset_base & 31) << PIO_SM0_PINCTRL_SIDESET_BASE_LSB);
}

static inline void sm_config_set_out_shift(pio_sm_config *c, bool shift_right, bool autopull, uint pull_threshold) {
    c->shiftctrl =
        (c->shiftctrl &
         ~(PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS |
           PIO_SM0_SHIFTCTRL_AUTOPULL_BITS |
           PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS)) |
        ((uint32_t) shift_right << PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_LSB) |
        ((uint32_t) autopull << PIO_SM0_SHIFTCTRL_AUTOPULL_LSB) |
        ((pull_threshold & 0x1fu) << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB);
}

static inline void sm_config_set_in_shift(pio_sm_config *c, bool shift_right, bool autopush, uint push_threshold) {
    c->shiftctrl =
        (c->shiftctrl &
         ~(PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS |
           PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS |
           PIO_SM0_SHIFTCTRL_PUSH_THRESH_BITS)) |
        ((uint32_t) shift_right << PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_LSB) |
        ((uint32_t) autopush << PIO_SM0_SHIFTCTRL_AUTOPUSH_LSB) |
        ((push_threshold & 0x1fu) << PIO_SM0_SHIFTCTRL_PUSH_THRESH_LSB);
}

static inline void sm_config_set_fifo_join(pio_sm_config *c, enum pio_fifo_join join) {
    c->shiftctrl =
        (c->shiftctrl & ~(PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS | PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS)) |
        (((uint) join) << PIO_SM0_SHIFTCTRL_FJOIN_TX_LSB);
}

static inline pio_sm_config pio_get_default_sm_config(void) {
    pio_sm_config c;
    c.clkdiv = 0;
    c.execctrl = 0;
    c.shiftctrl = 0;
    c.pinctrl = 0;
    sm_config_set_clkdiv_int_frac8(&c, 1, 0);
    sm_config_set_wrap(&c, 0, 31);
    sm_config_set_in_shift(&c, true, false, 32);
    sm_config_set_out_shift(&c, true, false, 32);
    return c;
}

int pio_add_program(PIO pio, const pio_program_t* program);
void pio_remove_program(PIO pio, const pio_program_t* program, uint loaded_offset);
void pio_gpio_init(PIO pio, uint pin);
int pio_sm_init(PIO pio, uint sm, uint initial_pc, const pio_sm_config *config);
void pio_sm_set_pindirs_with_mask(PIO pio, uint sm, uint32_t pindirs, uint32_t pin_mask);
void pio_sm_set_pins(PIO pio, uint sm, uint32_t pins);
uint pio_get_dreq(PIO pio, uint sm, bool is_tx);
int pio_claim_unused_sm(PIO pio, bool required);
void pio_sm_unclaim(PIO pio, uint sm);
void pio_sm_set_enabled(PIO pio, uint sm, bool enabled);

static inline uint8_t pio_sm_get_pc(PIO pio, uint sm) {
    return (uint8_t) (pio->sm[sm].addr & 0x1fu);
}

// The mock has no FIFO, and its dma_channel_start does not move data.
// Reporting the TX FIFO as primed models the state the library waits
// for before it enables the state machine.
static inline bool pio_sm_is_tx_fifo_empty(PIO pio, uint sm) {
    (void) pio;
    (void) sm;
    return false;
}

#ifdef __cplusplus
}
#endif

#endif /* _MOCK_HARDWARE_PIO_H */
