/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "IoAbstraction.h"

#ifdef __MBED__
#include <mbed.h>

void BasicIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    GpioWrapper* theGpio = this->allocatePinIfNeedBe(pin);
    if(theGpio != NULL) {
        if(mode == OUTPUT) {
            gpio_init_out(theGpio->getGpio(), (PinName)pin);
        }
        else {
            gpio_init_in_ex(theGpio->getGpio(), (PinName) pin, (PinMode)mode);
        }
        theGpio->setPinMode(mode);
    }
}

void BasicIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
    GpioWrapper* theGpio = this->allocatePinIfNeedBe(pin);
    if(theGpio != NULL) {
        gpio_write(theGpio->getGpio(), value);
    }
}

uint8_t BasicIoAbstraction::readValue(pinid_t pin) {
    GpioWrapper* theGpio = this->allocatePinIfNeedBe(pin);
    if(theGpio != NULL) {
        return gpio_read(theGpio->getGpio());
    }
    return 0;
}

void BasicIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
    auto gpio = allocatePinIfNeedBe(pin);
    if(gpio == NULL) return;
    if(gpio->getInterruptIn() == NULL) {
        gpio->setInterruptIn(new InterruptIn((PinName)pin));
    }
    auto intIn = gpio->getInterruptIn();
    intIn->mode(gpio->getPinMode() == INPUT_PULLUP ? PullUp : PullDown);
    if((mode & RISING) != 0) {
        intIn->rise(interruptHandler);
    }
    if((mode & FALLING) != 0) {
        intIn->fall(interruptHandler);
    }
}

void BasicIoAbstraction::writePort(pinid_t port, uint8_t portVal) {
    // unsupported on mbed at the moment
}

uint8_t BasicIoAbstraction::readPort(pinid_t port) {
    // unsupported on mbed at the moment
    return 0xff;
}

GpioWrapper *BasicIoAbstraction::allocatePinIfNeedBe(uint8_t pinToAlloc) {
    GpioWrapper* gpioWrapper = pinCache.getByKey(pinToAlloc);
    if(gpioWrapper == NULL) {
        pinCache.add(GpioWrapper(pinToAlloc));
        gpioWrapper = pinCache.getByKey(pinToAlloc);
    }
    return gpioWrapper;
}

IoAbstractionRef mbedAbstraction = NULL;
IoAbstractionRef internalDigitalIo() {
    if (mbedAbstraction == NULL) {
        mbedAbstraction = new BasicIoAbstraction();
    }
    return mbedAbstraction;
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

IoAbstractionRef arduinoAbstraction = NULL;
IoAbstractionRef ioUsingArduino() {
	noInterrupts();
	if (arduinoAbstraction == NULL) {
		arduinoAbstraction = new BasicIoAbstraction();
	}
	interrupts();
	return arduinoAbstraction;
}

#endif // __MBED__
