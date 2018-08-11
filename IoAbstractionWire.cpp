/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <IoAbstractionWire.h>
#include <Wire.h>


PCF8574IoAbstraction::PCF8574IoAbstraction(uint8_t addr) {
	this->address = addr;
	this->toWrite = 0x00;
	this->lastRead = 0;
	this->needsWrite = true;
}

void PCF8574IoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	if (mode == INPUT) {
		writeValue(pin, HIGH);
	}
	else {
		writeValue(pin, LOW);
	}
	needsWrite = true;
}

void PCF8574IoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	toWrite = writeBits(pin, value, toWrite);
	needsWrite = true;
}

uint8_t PCF8574IoAbstraction::readValue(uint8_t pin) {
	return (lastRead & (1 << pin)) ? HIGH : LOW;
}

void PCF8574IoAbstraction::runLoop(){
	if (needsWrite) {
		needsWrite = false;
		writeData();
	}
	readData();
}

void PCF8574IoAbstraction::writeData() {
	Wire.beginTransmission(address);
	Wire.write(toWrite);
	Wire.endTransmission();
	needsWrite = false;
}

uint8_t PCF8574IoAbstraction::readData() {
	Wire.requestFrom(address, (uint8_t)1);
	if (Wire.available()) {
		lastRead = Wire.read();
	}
	Wire.endTransmission();
	return lastRead;
}

BasicIoAbstraction* ioFrom8754(uint8_t addr) {
	return new PCF8574IoAbstraction(addr);
}
