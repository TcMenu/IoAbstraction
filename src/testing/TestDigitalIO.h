#if !defined(IOA_TEST_DIGIAL_IO_H)

#define IOA_TEST_DIGIAL_IO_H

#include "BasicIoAbstraction.h"

#define INPUT 0x01
#define INPUT_PULLUP 0x02
#define OUTPUT 0xff
#define RISING 0x01
#define FALLING 0x02
#define CHANGE 0x03
#define PROGMEM
#define HIGH 1
#define LOW 0

#define bitRead(value, bit) (((value) & (1 << (bit))) != 0)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#endif
