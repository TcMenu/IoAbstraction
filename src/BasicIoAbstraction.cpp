/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "IoAbstraction.h"

#ifdef __MBED__
#include <mbed.h>
#include <map>

void BasicIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    gpio_t* theGpio = this->allocatePinIfNeedBe(pin);
    if(theGpio != NULL) {
        if(mode == OUTPUT) {
            gpio_init_out(theGpio, (PinName)pin);
        }
        else {
            gpio_init_in_ex(theGpio, (PinName) pin, (PinMode)mode);
        }
    }
}

void BasicIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
    gpio_t* theGpio = this->allocatePinIfNeedBe(pin);
    if(theGpio != NULL) {
        gpio_write(theGpio, value);
    }
}

uint8_t BasicIoAbstraction::readValue(pinid_t pin) {
    gpio_t* theGpio = this->allocatePinIfNeedBe(pin);
    if(theGpio != NULL) {
        return gpio_read(theGpio);
    }
    return 0;
}

void BasicIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
    InterruptIn()
    uint8_t intPin = digitalPinToInterrupt(pin);
    ::attachInterrupt(intPin, interruptHandler, mode);
}

void BasicIoAbstraction::writePort(uint8_t port, uint8_t portVal) {
    // unsupported on mbed at the moment
}

uint8_t BasicIoAbstraction::readPort(uint8_t port) {
    // unsupported on mbed at the moment
    return 0xff;
}

gpio_t *BasicIoAbstraction::allocatePinIfNeedBe(uint8_t pinToAlloc) {
    gpio_t* gpio = pinCache[pinToAlloc];
    if(gpio == NULL) {
        gpio = new gpio_t;
        pinCache[pinToAlloc] = gpio;
    }
    return gpio;
}

#else // Assuming Arduino

void BasicIoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	pinMode(pin, mode);
}

void BasicIoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	digitalWrite(pin, value);
}

uint8_t BasicIoAbstraction::readValue(uint8_t pin) {
	return digitalRead(pin);
}

void BasicIoAbstraction::attachInterrupt(uint8_t pin, RawIntHandler interruptHandler, uint8_t mode) {
	uint8_t intPin = digitalPinToInterrupt(pin);
	::attachInterrupt(intPin, interruptHandler, mode);
}

void BasicIoAbstraction::writePort(uint8_t port, uint8_t portVal) {
	*portOutputRegister(digitalPinToPort(port)) = portVal;
}

uint8_t BasicIoAbstraction::readPort(uint8_t port) {
	return *portInputRegister(digitalPinToPort(port));
}

#endif // __MBED__
