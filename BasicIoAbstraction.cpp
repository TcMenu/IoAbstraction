#include "BasicIoAbstraction.h"
#include <Wire.h>

/*
 BasicIoAbstraction library, written by thecoderscorner.com
*/

void BasicIoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	pinMode(pin, mode);
}

void BasicIoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	digitalWrite(pin, value);
}

uint8_t BasicIoAbstraction::readValue(uint8_t pin) {
	return digitalRead(pin);
}



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
	needsWrite = true;
}

void PCF8574IoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	if (value) {
		toWrite |= (1 << pin);
	}
	else {
		uint8_t mask = (1 << pin) ^ 0xff;
		toWrite &= mask;
	}
	needsWrite = true;
}

uint8_t PCF8574IoAbstraction::readValue(uint8_t pin) {
	return (lastRead & (1 << pin)) == 1 ? HIGH : LOW;
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
}

BasicIoAbstraction* ioFrom8754(uint8_t addr) { 
	return new PCF8574IoAbstraction(addr); 
}

BasicIoAbstraction* ioUsingArduino() { 
	return new BasicIoAbstraction(); 
}
