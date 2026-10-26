#ifndef ESP32_DIGITAL_IO_H
#define ESP32_DIGITAL_IO_H

/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#if ((defined(ESP32) && defined(IOA_USE_ESP32_EXTRAS)) || defined(BUILD_FOR_ESP32_IDF))

#if defined(BUILD_FOR_ESP32_IDF)
#define bitRead(value, bit) (((value) & (1 << (bit))) != 0)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#endif

// if we are in ESP32 direct mode, then we must define the minimum things that digital IO uses.
#ifndef INPUT
#define INPUT 1
#define INPUT_PULLUP 2
#define INPUT_PULLDOWN 3
#define OUTPUT 4
#define RISING 1
#define FALLING 2
#define CHANGE 3
#define HIGH 1
#define LOW 0
#endif

#endif
#endif // ESP32_DIGITAL_IO_H
