/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

// pio-i2s.h embeds a copy of the pioasm output for src/pio-i2s-out.pio, so
// that clients can use the library without running the PIO assembler. These
// tests compare that copy against a freshly assembled one, and fail if an
// edit to the .pio source was not carried across.

#ifndef TEST_PIO_PROGRAM_TESTS_H
#define TEST_PIO_PROGRAM_TESTS_H

#ifdef PIOI2S_TEST_DEVICE

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "unity.h"
#include "pio-i2s.h"
#include "generated-pio-program.h"

static void testEmbeddedProgramMatchesGeneratedInstructions(void) {
    const uint16_t* generated = generatedProgramInstructions();
    size_t count = sizeof(PioI2S_out_program_instructions) /
        sizeof(PioI2S_out_program_instructions[0]);
    size_t i;

    TEST_ASSERT_EQUAL_size_t(generatedProgramLength(), count);

    for (i = 0; i < count; i++) {
        char message[48];
        snprintf(message, sizeof(message), "instruction %u", (unsigned) i);
        TEST_ASSERT_EQUAL_HEX16_MESSAGE(generated[i],
            PioI2S_out_program_instructions[i], message);
    }
}

static void testEmbeddedProgramMatchesGeneratedMetadata(void) {
    TEST_ASSERT_EQUAL_UINT(generatedProgramWrapTarget(),
        PioI2S_out_wrap_target);
    TEST_ASSERT_EQUAL_UINT(generatedProgramWrap(), PioI2S_out_wrap);
    TEST_ASSERT_EQUAL_UINT(generatedProgramEntryPoint(),
        PioI2S_out_offset_entry_point);
    TEST_ASSERT_EQUAL_UINT(generatedProgramVersion(), PioI2S_out_pio_version);
    TEST_ASSERT_EQUAL_UINT8(generatedProgramLength(),
        PioI2S_out_program.length);
    TEST_ASSERT_EQUAL_INT8(generatedProgramOrigin(), PioI2S_out_program.origin);
}

// A non-zero offset is used so that a mismatch in the wrap registers, which
// are encoded relative to where the program was loaded, is visible.
static void testEmbeddedProgramMatchesGeneratedDefaultConfig(void) {
    const uint offset = 12;
    pio_sm_config generated = generatedProgramDefaultConfig(offset);
    pio_sm_config embedded = PioI2S_out_program_get_default_config(offset);

    TEST_ASSERT_EQUAL_HEX32(generated.clkdiv, embedded.clkdiv);
    TEST_ASSERT_EQUAL_HEX32(generated.execctrl, embedded.execctrl);
    TEST_ASSERT_EQUAL_HEX32(generated.shiftctrl, embedded.shiftctrl);
    TEST_ASSERT_EQUAL_HEX32(generated.pinctrl, embedded.pinctrl);
}

static inline void runPioProgramTests(void) {
    RUN_TEST(testEmbeddedProgramMatchesGeneratedInstructions);
    RUN_TEST(testEmbeddedProgramMatchesGeneratedMetadata);
    RUN_TEST(testEmbeddedProgramMatchesGeneratedDefaultConfig);
}

#endif // PIOI2S_TEST_DEVICE
#endif // TEST_PIO_PROGRAM_TESTS_H
