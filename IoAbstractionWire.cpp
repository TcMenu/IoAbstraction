/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <Arduino.h>
#include <IoAbstractionWire.h>
#include <Wire.h>


PCF8574IoAbstraction::PCF8574IoAbstraction(uint8_t addr, uint8_t interruptPin) {
	this->address = addr;
	this->toWrite = 0x00;
	this->lastRead = 0;
	this->interruptPin = interruptPin;
	this->needsWrite = true;
}

void PCF8574IoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	if (mode == INPUT || mode == INPUT_PULLUP) {
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

void PCF8574IoAbstraction::attachInterrupt(uint8_t pin, RawIntHandler intHandler, uint8_t mode) {
	// if there's an interrupt pin set
	if(interruptPin == 0xff) return;

	pinMode(interruptPin, INPUT_PULLUP);

	// the 8574 INT is normally high, we need a FALLING state to know interrupt triggered 
	::attachInterrupt(digitalPinToInterrupt(interruptPin), intHandler, FALLING);
}


BasicIoAbstraction* ioFrom8574(uint8_t addr, uint8_t interruptPin) {
	return new PCF8574IoAbstraction(addr, interruptPin);
}


MCP23017IoAbstraction::MCP23017IoAbstraction(uint8_t address, uint8_t intPinA, uint8_t intPinB) {
	this->address = address;
	this->intPinA = intPinA;
	this->intPinB = intPinB;
	this->portCache = 0;
}

void MCP23017IoAbstraction::writeToPort(uint8_t reg, uint16_t command) {
	Wire.beginTransmission(address);
	Wire.write(reg);
	Wire.write(command>>8);
	Wire.write(command&0xff);
	Wire.endTransmission();
}

uint16_t MCP23017IoAbstraction::readFromPort(uint8_t reg, uint16_t command) {
	Wire.beginTransmission(address);
	Wire.write(reg);
	Wire.endTransmission(false);
	Wire.requestFrom(address, (uint8_t)2);
	Wire.endTransmission();
	return (Wire.read() << 8) | Wire.read();
}
