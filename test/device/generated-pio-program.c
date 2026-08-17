/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#include "pio-i2s-out.pio.h"
#include "generated-pio-program.h"

const uint16_t* generatedProgramInstructions(void) {
    return PioI2S_out_program_instructions;
}

uint8_t generatedProgramLength(void) {
    return PioI2S_out_program.length;
}

int8_t generatedProgramOrigin(void) {
    return PioI2S_out_program.origin;
}

uint generatedProgramWrapTarget(void) {
    return PioI2S_out_wrap_target;
}

uint generatedProgramWrap(void) {
    return PioI2S_out_wrap;
}

uint generatedProgramEntryPoint(void) {
    return PioI2S_out_offset_entry_point;
}

uint generatedProgramVersion(void) {
    return PioI2S_out_pio_version;
}

pio_sm_config generatedProgramDefaultConfig(uint offset) {
    return PioI2S_out_program_get_default_config(offset);
}
