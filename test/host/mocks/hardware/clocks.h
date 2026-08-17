/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef _MOCK_HARDWARE_CLOCKS_H
#define _MOCK_HARDWARE_CLOCKS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum clock_index {
    clk_gpout0 = 0,
    clk_gpout1,
    clk_gpout2,
    clk_gpout3,
    clk_ref,
    clk_sys,
    clk_peri,
    clk_usb,
    clk_adc,
    clk_rtc,
    CLK_COUNT
};

uint32_t clock_get_hz(enum clock_index clock_index);

void mockClockSetSysSpeed(uint32_t frequency);

#ifdef __cplusplus
}
#endif

#endif /* _MOCK_HARDWARE_CLOCKS_H */
