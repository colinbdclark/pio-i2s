/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef _MOCK_PICO_STDLIB_H
#define _MOCK_PICO_STDLIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// The pico-sdk uses `uint` (unsigned int) throughout its public API.
typedef unsigned int uint;

// The real panic() halts the chip; the mock exits the process instead.
void __attribute__((noreturn)) panic(const char* fmt, ...);

static inline void tight_loop_contents(void) {}

void mockReset(void);

#ifdef __cplusplus
}
#endif

#endif /* _MOCK_PICO_STDLIB_H */
