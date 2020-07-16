/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef TCLIBRARYDEV_PLATFORMDETERMINATION_H
#define TCLIBRARYDEV_PLATFORMDETERMINATION_H

// the purpose of this file is to determine the actual main environment, which has got a bit more tricky now
// that some Arduino devices are actually mbed based, but not standard enough to use mbed as usual.
// As more Arduino mbed devices become available, I imagine this list will grow, and we may revisit if
// it's better to work out how to include the mbed namespace and use our mbed layer instead.

#if defined(ARDUINO_ARDUINO_NANO33BLE)
// here we're in a hybrid of mbed and Arduino basically. We treat all abstractions as Arduino though.
#include <Arduino.h>
# define IOA_USE_ARDUINO
# define IOA_ARDUINO_MBED
# define IOA_ANALOGIN_RES 12
# define IOA_ANALOGOUT_RES 8
typedef uint32_t pinid_t;
#elif defined(__MBED__)
// here we are in full mbed mode
# define IOA_USE_MBED
#include <mbed.h>
typedef uint32_t pinid_t;
#else
// here we are in full arduino mode (AVR, MKR, ESP etc).
# define IOA_USE_ARDUINO
#include <Arduino.h>
typedef uint8_t pinid_t;
#define IOA_DEVICE_HAS_PORTS

#if defined(ARDUINO_ARCH_SAMD) && !defined(IO_MKR_FORCE_LOWRES_ANALOG)
# define IOA_ANALOGIN_RES 12
# define IOA_ANALOGOUT_RES 10
#elif defined(ESP32)
# define IOA_ANALOGIN_RES 12
# define IOA_ANALOGOUT_RES 8
#else
# define IOA_ANALOGIN_RES 10
# define IOA_ANALOGOUT_RES 8
#endif // SAMD ARCH

#endif // __MBED__

#endif //TCLIBRARYDEV_PLATFORMDETERMINATION_H
