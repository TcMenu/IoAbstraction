/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef TCLIBRARYDEV_PLATFORMDETERMINATION_H
#define TCLIBRARYDEV_PLATFORMDETERMINATION_H

// You can add your own local definitions header file here, this enables you to adjust build flags in environments
// where there is no easy way to do so with compiler options. Just create an include file "io_local_definitions.h"
// at the top level of your project source tree.
#if defined __has_include
#  if __has_include ("zio_local_definitions.h")
#    include "zio_local_definitions.h"
#  endif
#endif // has include "io_local_definitions"

// when not on mbed, we need to load Arduino.h to get the right defines for some boards.
#if !defined(__MBED__) && !defined(BUILD_FOR_PICO_CMAKE)
#include <Arduino.h>
#endif

// the purpose of this file is to determine the actual main environment, which has got a bit more tricky now
// that some Arduino devices are actually mbed based, but not standard enough to use mbed as usual.
// As more Arduino mbed devices become available, I imagine this list will grow, and we may revisit if
// it's better to work out how to include the mbed namespace and use our mbed layer instead.

// If you have a board that's not properly mapped, please raise an issue and we'll see if it's possible to add it.
// This file is shared across IoTaskManager and IoAbstraction

// list of devices is pulled from https://github.com/arduino/ArduinoCore-mbed/blob/master/full.variables
// set TMIOA_FORCE_ARDUINO_MBED to force IoAbstraction to use Arduino-mbed mode.
#if defined(ARDUINO_PICO_REVISION)
// on the Earl Philhower pico implementation
#include <Arduino.h>
# define IOA_USE_ARDUINO
# define IOA_ANALOGIN_RES 12
# define IOA_ANALOGOUT_RES 10
typedef uint8_t pinid_t;
#elif defined(ARDUINO_NANO_RP2040_CONNECT) || \
    defined(ARDUINO_ARDUINO_NANO33BLE) || \
    defined(ARDUINO_RASPBERRY_PI_PICO) || \
    defined(ARDUINO_PORTENTA_H7_M7) || \
    defined(ARDUINO_PORTENTA_H7_M4) || \
    defined(ARDUINO_EDGE_CONTROL) || \
    defined(ARDUINO_NICLA) || \
    defined(ARDUINO_NICLA_VISION) || \
    defined(TMIOA_FORCE_ARDUINO_MBED) || \
    defined(ARDUINO_ARCH_MBED)
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
#elif defined(BUILD_FOR_PICO_CMAKE)
#include <pico/stdlib.h>
typedef uint8_t pinid_t;
#define pgm_read_byte_near(x) (*(x))
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
#elif defined(ESP8266)
# define IOA_ANALOGIN_RES 10
# define IOA_ANALOGOUT_RES 10
#elif defined(ARDUINO_ARCH_RENESAS_UNO) && !defined(IO_MKR_FORCE_LOWRES_ANALOG)
#define IOA_ANALOGIN_RES 14
#define IOA_ANALOGOUT_RES 12
#else
# define IOA_ANALOGIN_RES 10
# define IOA_ANALOGOUT_RES 8
#endif // SAMD ARCH

#endif // __MBED__

#endif //TCLIBRARYDEV_PLATFORMDETERMINATION_H
