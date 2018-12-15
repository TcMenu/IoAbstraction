/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */
#ifndef _NEGATING_IO_ABSTRACTION_
#define _NEGATING_IO_ABSTRACTION_

#include "BasicIoAbstraction.h"

/**
 * This implementation of IoAbstraction negates all the pin operations on the given abstraction, both for read and write.
 * Useful when there is a need to invert the meaning such as when dealing with both PULL UP and PULL DOWN switches at the
 * same time on different IO devices.
 */
class NegatingIoAbstraction : public BasicIoAbstraction {
private:
    IoAbstractionRef delegate;
public:
    NegatingIoAbstraction(IoAbstractionRef toInvert) { delegate = toInvert; }
   	
    void pinDirection(uint8_t pin, uint8_t mode) override { 
        delegate->pinDirection(pin, mode); 
    }
	
    void writeValue(uint8_t pin, uint8_t value) override {
        delegate->writeValue(pin, !value); 
    }
	
    uint8_t readValue(uint8_t pin) override {
        return !delegate->readValue(pin);
    }
	
    void attachInterrupt(uint8_t pin, RawIntHandler interruptHandler, uint8_t mode) override {
        delegate->attachInterrupt(pin, interruptHandler, mode); 
    }
  	
    bool runLoop() override {
        return delegate->runLoop();
    }

    void writePort(uint8_t pin, uint8_t portVal) override {
        delegate->writePort(pin, ~portVal);
    }

    uint8_t readPort(uint8_t pin) override {
        return ~(delegate->readPort(pin));
    }
};

#endif // _NEGATING_IO_ABSTRACTION_