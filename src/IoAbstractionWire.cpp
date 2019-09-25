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
	this->pinsConfiguredRead = false;
}

void PCF8574IoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	if (mode == INPUT || mode == INPUT_PULLUP) {
		pinsConfiguredRead = true;
		writeValue(pin, HIGH);
	}
	else {
		writeValue(pin, LOW);
	}
	needsWrite = true;
}

void PCF8574IoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	bitWrite(toWrite, pin, value);
	needsWrite = true;
}

uint8_t PCF8574IoAbstraction::readValue(uint8_t pin) {
	return (lastRead & (1 << pin)) ? HIGH : LOW;
}

uint8_t PCF8574IoAbstraction::readPort(uint8_t /*pin*/) {
	return lastRead;
}

void PCF8574IoAbstraction::writePort(uint8_t /*pin*/, uint8_t value) {
	toWrite = value;
	needsWrite = true;
}

bool PCF8574IoAbstraction::runLoop(){
	bool writeOk = true;
	if (needsWrite) {
		needsWrite = false;
		Wire.beginTransmission(address);
		Wire.write(toWrite);
		writeOk = Wire.endTransmission() == 0;
	}
	
	if(pinsConfiguredRead) {
		Wire.requestFrom(address, (uint8_t)1);
		if (Wire.available()) {
			lastRead = Wire.read();
		}
	}
	return writeOk;
}

void PCF8574IoAbstraction::attachInterrupt(uint8_t /*pin*/, RawIntHandler intHandler, uint8_t /*mode*/) {
	// if there's an interrupt pin set
	if(interruptPin == 0xff) return;

	pinMode(interruptPin, INPUT_PULLUP);

	// the 8574 INT is normally high, we need a FALLING state to know interrupt triggered 
	::attachInterrupt(digitalPinToInterrupt(interruptPin), intHandler, FALLING);
}

BasicIoAbstraction* ioFrom8574(uint8_t addr, uint8_t interruptPin) {
	return new PCF8574IoAbstraction(addr, interruptPin);
}

MCP23017IoAbstraction::MCP23017IoAbstraction(uint8_t address, Mcp23xInterruptMode intMode, uint8_t intPinA, uint8_t intPinB) {
	this->address = address;
	this->intPinA = intPinA;
	this->intPinB = intPinB;
	this->intMode = intMode;
	this->toWrite = this->lastRead = 0;
	this->needsInit = true;
	this->portFlags = 0;
}

void MCP23017IoAbstraction::initDevice() {
	uint8_t controlReg = (readFromDevice(IOCON_ADDR) & 0xff);
	
	if(intPinB == 0xff && intPinA != 0xff) {
		bitSet(controlReg, IOCON_MIRROR_BIT);
	}
	else if(intPinA != 0xff && intPinB != 0xff) {
		bitClear(controlReg, IOCON_MIRROR_BIT);
	}

	bitClear(controlReg, IOCON_BANK_BIT);
	bitClear(controlReg, IOCON_SEQOP_BIT);

	uint16_t regToWrite = controlReg | (((uint16_t)controlReg) << 8);
	writeToDevice(IOCON_ADDR, regToWrite);

	portFlags = 0;
	needsInit = false;
}

void MCP23017IoAbstraction::toggleBitInRegister(uint8_t regAddr, uint8_t theBit, bool value) {
	uint16_t reg = readFromDevice(regAddr);
	bitWrite(reg, theBit, value);
	// for debugging to see the commands being sent, uncomment below
	//Serial.print("toggle call 0x"); Serial.print(reg, HEX); Serial.print(" pin "); Serial.print(theBit); Serial.print(" toggle "); Serial.print(value);
	//Serial.print(" reg "); Serial.println(regAddr, HEX);
	// end debugging code
	writeToDevice(regAddr, reg);
}

void MCP23017IoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	if(needsInit) initDevice();

	toggleBitInRegister(IODIR_ADDR, pin, (mode == INPUT || mode == INPUT_PULLUP));
	toggleBitInRegister(GPPU_ADDR, pin, mode == INPUT_PULLUP);

	bitSet(portFlags, (pin < 8) ? READER_PORTA_BIT : READER_PORTB_BIT);
}

void MCP23017IoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	if(needsInit) initDevice();

	bitWrite(toWrite, pin, value);
	bitSet(portFlags, (pin < 8) ? CHANGE_PORTA_BIT : CHANGE_PORTB_BIT);
}

uint8_t MCP23017IoAbstraction::readValue(uint8_t pin) {
	return bitRead(lastRead, pin);
}

uint8_t MCP23017IoAbstraction::readPort(uint8_t pin) {
	return (pin < 8) ? (lastRead & 0xff) : (lastRead >> 8);
}

void MCP23017IoAbstraction::writePort(uint8_t pin, uint8_t value) {
	if(pin < 8) {
		toWrite &= 0xff00; 
		toWrite |= value;
		bitSet(portFlags, CHANGE_PORTA_BIT);
	}
	else {
		toWrite &= 0x00ff; 
		toWrite |= ((uint16_t)value << 8);
		bitSet(portFlags, CHANGE_PORTB_BIT);
	}
}

bool MCP23017IoAbstraction::runLoop() {
	if(needsInit) initDevice();

	bool writeOk = true;

	bool flagA = bitRead(portFlags, CHANGE_PORTA_BIT);
	bool flagB = bitRead(portFlags, CHANGE_PORTB_BIT);
	if(flagA && flagB) // write on both ports
		writeOk = writeToDevice(OUTLAT_ADDR, toWrite);
	else if(flagA) 
		writeOk = writeToDevice8(OUTLAT_ADDR, toWrite);
	else if(flagB)
		writeOk = writeToDevice8(OUTLAT_ADDR + 1, toWrite >> 8);

	bitClear(portFlags, CHANGE_PORTA_BIT);
	bitClear(portFlags, CHANGE_PORTB_BIT);

	flagA = bitRead(portFlags, READER_PORTA_BIT);
	flagB = bitRead(portFlags, READER_PORTB_BIT);
	if(flagA && flagB)
		lastRead = readFromDevice(GPIO_ADDR);
	else if(flagA)
		lastRead = readFromDevice8(GPIO_ADDR);
	else if(flagB)
		lastRead = readFromDevice8(GPIO_ADDR + 1) << 8;

	return writeOk;
}

bool MCP23017IoAbstraction::writeToDevice(uint8_t reg, uint16_t command) {
	Wire.beginTransmission(address);
	Wire.write(reg);

	// write port A then port B.
	Wire.write((uint8_t)command);
	Wire.write((uint8_t)(command>>8));
	return Wire.endTransmission() == 0;
}

bool MCP23017IoAbstraction::writeToDevice8(uint8_t reg, uint8_t command) {
	Wire.beginTransmission(address);
	Wire.write(reg);

	// write only one port.
	Wire.write((uint8_t)command);
	return Wire.endTransmission() == 0;
}

uint16_t MCP23017IoAbstraction::readFromDevice(uint8_t reg) {
	Wire.beginTransmission(address);
	Wire.write(reg);
	Wire.endTransmission(false);
	
	Wire.requestFrom(address, (uint8_t)2);
	// read will get port A first then port B.
	uint8_t portA = Wire.read();
	uint16_t portB = (Wire.read() << 8);
	return portA | portB;
}

uint8_t MCP23017IoAbstraction::readFromDevice8(uint8_t reg) {
	Wire.beginTransmission(address);
	Wire.write(reg);
	Wire.endTransmission(false);
	
	Wire.requestFrom(address, (uint8_t)1);
	return Wire.read();
}

void MCP23017IoAbstraction::attachInterrupt(uint8_t pin, RawIntHandler intHandler, uint8_t mode) {
	// only if there's an interrupt pin set
	if(intPinA == 0xff) return;

	uint8_t pm = (intMode == ACTIVE_HIGH_OPEN || intMode == ACTIVE_LOW_OPEN) ? INPUT_PULLUP : INPUT;
	uint8_t im = (intMode == ACTIVE_HIGH || intMode == ACTIVE_HIGH_OPEN) ? RISING : FALLING;
	if(intPinB == 0xff) {
		pinMode(intPinA, pm);
		::attachInterrupt(digitalPinToInterrupt(intPinA), intHandler, im);
	}
	else {
		pinMode(intPinA, pm);
		pinMode(intPinB, pm);
		::attachInterrupt(digitalPinToInterrupt(intPinA), intHandler, im);
		::attachInterrupt(digitalPinToInterrupt(intPinB), intHandler, im);
	}

	toggleBitInRegister(GPINTENA_ADDR, pin, true);
	toggleBitInRegister(INTCON_ADDR, pin, mode != CHANGE);
	toggleBitInRegister(DEFVAL_ADDR, pin, mode == FALLING);
}

void MCP23017IoAbstraction::setInvertInputPin(uint8_t pin, bool shouldInvert) {
    toggleBitInRegister(IPOL_ADDR, pin, shouldInvert);
}

IoAbstractionRef ioFrom23017(uint8_t addr) {
	return ioFrom23017IntPerPort(addr, NOT_ENABLED, 0xff, 0xff);
}

IoAbstractionRef ioFrom23017(uint8_t addr, Mcp23xInterruptMode intMode, uint8_t interruptPin) {
	return ioFrom23017IntPerPort(addr, intMode, interruptPin, 0xff);
}

IoAbstractionRef ioFrom23017IntPerPort(uint8_t addr, Mcp23xInterruptMode intMode, uint8_t interruptPinA, uint8_t interruptPinB) {
	return new MCP23017IoAbstraction(addr, intMode, interruptPinA, interruptPinB);
}