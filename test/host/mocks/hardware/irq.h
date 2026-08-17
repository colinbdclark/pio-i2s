/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef _MOCK_HARDWARE_IRQ_H
#define _MOCK_HARDWARE_IRQ_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*irq_handler_t)(void);

void irq_set_exclusive_handler(uint num, irq_handler_t handler);

void irq_set_enabled(uint num, bool enabled);

void irq_remove_handler(uint num, irq_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* _MOCK_HARDWARE_IRQ_H */
