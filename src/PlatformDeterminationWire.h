/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef IOA_PLATFORMDETERMINATIONWIRE_H
#define IOA_PLATFORMDETERMINATIONWIRE_H

#include "PlatformDetermination.h"

//
// Here we work out what wire looks like on this board! Becoming non trivial these days!
//
#ifdef IOA_USE_MBED
#include <i2c_api.h>
typedef I2C* WireType;

class I2CLocker {
private:
    I2C* i2cPtr;
public:
    I2CLocker(I2C* i2c) {
        i2cPtr = i2c;
        i2c->lock();
    }

    ~I2CLocker() {
        i2cPtr->unlock();
    }
};

#else
#include <Wire.h>
typedef TwoWire* WireType;
#endif // IOA_USE_MBED

#endif //IOA_PLATFORMDETERMINATIONWIRE_H
