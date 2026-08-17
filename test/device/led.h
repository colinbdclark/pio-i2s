/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

#ifndef TEST_LED_H
#define TEST_LED_H

#include "pico/stdlib.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#else
#include "hardware/gpio.h"
#endif

typedef struct LED {
    bool state;
} LED;

static inline void ledInit(LED* led) {
    led->state = false;
    #ifdef CYW43_WL_GPIO_LED_PIN
        cyw43_arch_init();
    #else
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    #endif
}

static inline void ledSet(LED* led, bool state) {
    if (led->state == state) {
        return;
    }

    led->state = state;

    #ifdef CYW43_WL_GPIO_LED_PIN
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
    #else
        gpio_put(PICO_DEFAULT_LED_PIN, state);
    #endif
}

static inline void ledToggle(LED* led) {
    ledSet(led, !led->state);
}

#endif // TEST_LED_H
