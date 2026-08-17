/*
* Copyright 2025-6 The pio-i2s Contributors.
* Portions derived from the Pico SDK (BSD-3-Clause, Copyright (c) 2024 Raspberry Pi Ltd.)
* Licensed under the BSD-3 License.
*/

#ifndef _MOCK_HARDWARE_DMA_H
#define _MOCK_HARDWARE_DMA_H

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

#define NUM_DMA_CHANNELS 12u
#define NUM_DMA_IRQS 2u

#define DREQ_FORCE 0x3fu

#define DMA_IRQ_0 11u
#define DMA_IRQ_1 12u

#define DMA_CH0_CTRL_TRIG_BSWAP_BITS        _u(0x01000000)
#define DMA_CH0_CTRL_TRIG_IRQ_QUIET_BITS    _u(0x00800000)
#define DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB      _u(17)
#define DMA_CH0_CTRL_TRIG_TREQ_SEL_BITS     _u(0x007e0000)
#define DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB      _u(13)
#define DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS     _u(0x0001e000)
#define DMA_CH0_CTRL_TRIG_RING_SEL_BITS     _u(0x00001000)
#define DMA_CH0_CTRL_TRIG_RING_SIZE_LSB     _u(8)
#define DMA_CH0_CTRL_TRIG_RING_SIZE_BITS    _u(0x00000f00)
#define DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS   _u(0x00000040)
#define DMA_CH0_CTRL_TRIG_INCR_READ_BITS    _u(0x00000010)
#define DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB     _u(2)
#define DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS    _u(0x0000000c)
#define DMA_CH0_CTRL_TRIG_EN_BITS           _u(0x00000001)

enum dma_channel_transfer_size {
    DMA_SIZE_8 = 0,
    DMA_SIZE_16 = 1,
    DMA_SIZE_32 = 2
};

typedef struct {
    uint32_t ctrl;
} dma_channel_config;

// Mirrors the subset of dma_channel_hw_t registers used by the
// library and tests.
typedef struct {
    io_rw_32 read_addr;
    io_rw_32 write_addr;
    io_rw_32 transfer_count;
    io_rw_32 ctrl_trig;
    io_rw_32 al1_ctrl;
    io_rw_32 al3_read_addr_trig;
} dma_channel_hw_t;

typedef struct {
    dma_channel_hw_t ch[NUM_DMA_CHANNELS];
} dma_hw_t;

extern dma_hw_t dma_hw_instance;
#define dma_hw (&dma_hw_instance)

// Mirrors the subset of the DMA debug registers used by the tests.
// dbg_tcr holds the transfer count reload value written at configure
// time; the live transfer_count register only reflects it once the
// channel has been triggered.
typedef struct {
    io_rw_32 dbg_ctdreq;
    io_rw_32 dbg_tcr;
} dma_debug_channel_hw_t;

typedef struct {
    dma_debug_channel_hw_t ch[NUM_DMA_CHANNELS];
} dma_debug_hw_t;

extern dma_debug_hw_t dma_debug_hw_instance;
#define dma_debug_hw (&dma_debug_hw_instance)

static inline dma_channel_hw_t *dma_channel_hw_addr(uint channel) {
    return &dma_hw->ch[channel];
}

static inline void channel_config_set_read_increment(dma_channel_config *c, bool incr) {
    c->ctrl = incr ?
        (c->ctrl | DMA_CH0_CTRL_TRIG_INCR_READ_BITS) :
        (c->ctrl & ~DMA_CH0_CTRL_TRIG_INCR_READ_BITS);
}

static inline void channel_config_set_write_increment(dma_channel_config *c, bool incr) {
    c->ctrl = incr ?
        (c->ctrl | DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS) :
        (c->ctrl & ~DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS);
}

static inline void channel_config_set_dreq(dma_channel_config *c, uint dreq) {
    c->ctrl = (c->ctrl & ~DMA_CH0_CTRL_TRIG_TREQ_SEL_BITS) |
              (dreq << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB);
}

static inline void channel_config_set_chain_to(dma_channel_config *c, uint chain_to) {
    c->ctrl = (c->ctrl & ~DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS) |
              (chain_to << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB);
}

static inline void channel_config_set_transfer_data_size(dma_channel_config *c, enum dma_channel_transfer_size size) {
    c->ctrl = (c->ctrl & ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS) |
              (((uint) size) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);
}

static inline void channel_config_set_ring(dma_channel_config *c, bool write, uint size_bits) {
    c->ctrl = (c->ctrl & ~(DMA_CH0_CTRL_TRIG_RING_SIZE_BITS | DMA_CH0_CTRL_TRIG_RING_SEL_BITS)) |
              (size_bits << DMA_CH0_CTRL_TRIG_RING_SIZE_LSB) |
              (write ? DMA_CH0_CTRL_TRIG_RING_SEL_BITS : 0);
}

static inline void channel_config_set_irq_quiet(dma_channel_config *c, bool irq_quiet) {
    c->ctrl = irq_quiet ?
        (c->ctrl | DMA_CH0_CTRL_TRIG_IRQ_QUIET_BITS) :
        (c->ctrl & ~DMA_CH0_CTRL_TRIG_IRQ_QUIET_BITS);
}

static inline void channel_config_set_bswap(dma_channel_config *c, bool bswap) {
    c->ctrl = bswap ?
        (c->ctrl | DMA_CH0_CTRL_TRIG_BSWAP_BITS) :
        (c->ctrl & ~DMA_CH0_CTRL_TRIG_BSWAP_BITS);
}

static inline void channel_config_set_enable(dma_channel_config *c, bool enable) {
    c->ctrl = enable ?
        (c->ctrl | DMA_CH0_CTRL_TRIG_EN_BITS) :
        (c->ctrl & ~DMA_CH0_CTRL_TRIG_EN_BITS);
}

static inline dma_channel_config dma_channel_get_default_config(uint channel) {
    dma_channel_config c = {0};
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_FORCE);
    channel_config_set_chain_to(&c, channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_ring(&c, false, 0);
    channel_config_set_bswap(&c, false);
    channel_config_set_irq_quiet(&c, false);
    channel_config_set_enable(&c, true);
    return c;
}

void dma_channel_configure(uint channel, const dma_channel_config *config,
    volatile void* write_addr, const volatile void* read_addr,
    uint transfer_count, bool trigger);

void dma_channel_start(uint channel);
void dma_channel_cleanup(uint channel);
int dma_claim_unused_channel(bool required);
void dma_channel_unclaim(uint channel);

void dma_irqn_set_channel_enabled(uint irq_index, uint channel, bool enabled);
void dma_irqn_acknowledge_channel(uint irq_index, uint channel);

#ifdef __cplusplus
}
#endif

#endif /* _MOCK_HARDWARE_DMA_H */
