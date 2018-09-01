/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "IoAbstraction.h"

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
	attachInterrupt(digitalPinToInterrupt(pin), interruptHandler, mode);
}

void BasicIoAbstraction::writePort(uint8_t port, uint8_t portVal) {
	*portOutputRegister(digitalPinToPort(port)) = portVal;
}

uint8_t BasicIoAbstraction::readPort(uint8_t port) {
	return *portInputRegister(digitalPinToPort(port));
}
