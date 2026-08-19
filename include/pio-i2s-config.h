/*
* Copyright 2025-6 The pio-i2s Contributors.
* Licensed under the BSD-3 License.
*/

/*
* To override the configuration definitions within this file,
* put a directory containing your own pio-i2s-config.h file
* on the include path ahead of the pio-i2s's include directory.
*/
#ifndef PIOI2S_CONFIG_H
#define PIOI2S_CONFIG_H

/**
 * @brief Whether PioI2S_nextOutputBuffer zeroes the buffer before returning it.
 */
#ifndef PioI2S_ZERO_ON_UNDERRUN
#define PioI2S_ZERO_ON_UNDERRUN 0
#endif

#endif /* PIOI2S_CONFIG_H */
