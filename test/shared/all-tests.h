/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_ALL_TESTS_H
#define TEST_ALL_TESTS_H

#include "conversion-tests.h"
#include "buffer-tests.h"
#include "clock-division-tests.h"
#include "init-tests.h"

#ifdef PIOI2S_TEST_HOST
#include "panic-tests.h"
#endif

#ifdef PIOI2S_TEST_DEVICE
#include "hardware-tests.h"
#include "loopback-tests.h"
#endif

static inline void runAllTests(void) {
    runConversionTests();
    runBufferTests();
    runClockDivisionTests();
#ifdef PIOI2S_TEST_HOST
    runPanicTests();
#endif
    runInitTests();
#ifdef PIOI2S_TEST_DEVICE
    runHardwareTests();
    runLoopbackTests();
#endif
}

#endif // TEST_ALL_TESTS_H
