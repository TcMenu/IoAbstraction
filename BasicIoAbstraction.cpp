#include "BasicIoAbstraction.h"
#include <Wire.h>

#define LATCH_TIME 5

/*
 BasicIoAbstraction library, Apache 2.0 license, written by thecoderscorner.com
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
	else {
		writeValue(pin, LOW);
	}
	needsWrite = true;
}

uint8_t writeBits(uint8_t pin, uint8_t value, uint8_t existingValue) {
	uint8_t toWrite = existingValue;
	if (value) {
		toWrite |= (1 << pin);
	}
	else {
		uint8_t mask = (1 << pin) ^ 0xff;
		toWrite &= mask;
	}
	return toWrite;
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
}

ShiftRegisterIoAbstraction::ShiftRegisterIoAbstraction(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t readClockEnaPin, uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin) {
	needsWrite = true;
	toWrite = 0;

	this->readClkEnablePin = readClockEnaPin;
	this->readClockPin = readClockPin;
	this->writeClockPin = writeClockPin;
	this->readDataPin = readDataPin;
	this->writeDataPin = writeDataPin;
	this->readLatchPin = readLatchPin;
	this->writeLatchPin = writeLatchPin;

	if (writeDataPin != 0xff) {
		pinMode(writeLatchPin, OUTPUT);
		pinMode(writeDataPin, OUTPUT);
		pinMode(writeClockPin, OUTPUT);
		digitalWrite(writeLatchPin, LOW);
	}

	if (readLatchPin != 0xff) {
		pinMode(readLatchPin, OUTPUT);
		pinMode(readDataPin, INPUT);
		pinMode(readClockPin, OUTPUT);
		pinMode(readClockEnaPin, OUTPUT);
		digitalWrite(readLatchPin, HIGH);
	}
}

void ShiftRegisterIoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	// ignored, this implementation has hardwired inputs and outputs - inputs are 0-23, outputs are 24 onwards
}

void ShiftRegisterIoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	pin = pin - 24;
	if (pin < 0) return;

	toWrite = writeBits(pin, value, toWrite);
	needsWrite = true;
}

uint8_t ShiftRegisterIoAbstraction::readValue(uint8_t pin) {
	return (lastRead & (1 << pin)) ? HIGH : LOW;
}

void ShiftRegisterIoAbstraction::runLoop() {
	if (readDataPin != 0xff) {
		digitalWrite(readClkEnablePin, HIGH);
		digitalWrite(readLatchPin, LOW);
		delayMicroseconds(LATCH_TIME);
		digitalWrite(readLatchPin, HIGH);
		digitalWrite(readClkEnablePin, LOW);

		lastRead = shiftIn(readDataPin, readClockPin, MSBFIRST);
	}
	
	if (writeDataPin != 0xff && needsWrite) {
		digitalWrite(writeLatchPin, LOW);
		delayMicroseconds(LATCH_TIME);
		shiftOut(writeDataPin, writeClockPin, MSBFIRST, toWrite);
		needsWrite = false;
		digitalWrite(writeLatchPin, HIGH);
	}
}

BasicIoAbstraction* ioFrom8754(uint8_t addr) { 
	return new PCF8574IoAbstraction(addr); 
}

BasicIoAbstraction* outputOnlyFromShiftRegister(uint8_t writeClkPin, uint8_t dataPin, uint8_t latchPin) {
	return new ShiftRegisterIoAbstraction(0xff, 0xff, 0xff, 0xff, writeClkPin, dataPin, latchPin);
}

BasicIoAbstraction* inputOnlyFromShiftRegister(uint8_t readClkPin, uint8_t readClkEnaPin, uint8_t dataPin, uint8_t latchPin) {
	return new ShiftRegisterIoAbstraction(readClkPin, dataPin, latchPin, readClkEnaPin, 0xff, 0xff, 0xff);
}

BasicIoAbstraction* inputOutputFromShiftRegister(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t readClockEnaPin, uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin) {
	return new ShiftRegisterIoAbstraction(readClockPin, readDataPin, readLatchPin, readClockEnaPin, writeClockPin, writeDataPin, writeLatchPin);
}

BasicIoAbstraction* ioUsingArduino() { 
	return new BasicIoAbstraction(); 
}
