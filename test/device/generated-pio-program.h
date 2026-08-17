/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_GENERATED_PIO_PROGRAM_H
#define TEST_GENERATED_PIO_PROGRAM_H

#include <stddef.h>
#include <stdint.h>
#include "hardware/pio.h"

// The pioasm output for src/pio-i2s-out.pio declares the same names as the
// copy embedded in pio-i2s.h, so it is compiled in its own translation unit
// and reached through these accessors.

const uint16_t* generatedProgramInstructions(void);

uint8_t generatedProgramLength(void);

int8_t generatedProgramOrigin(void);

uint generatedProgramWrapTarget(void);

uint generatedProgramWrap(void);

uint generatedProgramEntryPoint(void);

uint generatedProgramVersion(void);

pio_sm_config generatedProgramDefaultConfig(uint offset);

#endif // TEST_GENERATED_PIO_PROGRAM_H
