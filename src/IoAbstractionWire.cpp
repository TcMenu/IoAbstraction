/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <IoAbstractionWire.h>

PCF8574IoAbstraction::PCF8574IoAbstraction(uint8_t addr, uint8_t interruptPin, WireType wireImplementation) {
	this->wireImpl = wireImplementation;
	this->address = addr;
	this->toWrite = 0x00;
	this->lastRead = 0;
	this->interruptPin = interruptPin;
	this->needsWrite = true;
	this->pinsConfiguredRead = false;
}

void PCF8574IoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
	if (mode == INPUT || mode == INPUT_PULLUP) {
		pinsConfiguredRead = true;
		writeValue(pin, HIGH);
	}
	else {
		writeValue(pin, LOW);
	}
	needsWrite = true;
}

void PCF8574IoAbstraction::writeValue(pinid_t pin, uint8_t value) {
	bitWrite(toWrite, pin, value);
	needsWrite = true;
}

uint8_t PCF8574IoAbstraction::readValue(pinid_t pin) {
	return (lastRead & (1 << pin)) ? HIGH : LOW;
}

uint8_t PCF8574IoAbstraction::readPort(pinid_t /*pin*/) {
	return lastRead;
}

void PCF8574IoAbstraction::writePort(pinid_t /*pin*/, uint8_t value) {
	toWrite = value;
	needsWrite = true;
}

#ifdef __MBED__
bool PCF8574IoAbstraction::runLoop(){
    I2CLocker locker(wireImpl);
    bool writeOk = true;
    if (needsWrite) {
        needsWrite = false;
        writeOk = wireImpl->write(address, (char*)&toWrite, 1) == 0;
    }

    if(pinsConfiguredRead) {
        writeOk = writeOk || (wireImpl->read(address, (char*)&lastRead, 1) == 0);
    }
    return writeOk;
}
#else
bool PCF8574IoAbstraction::runLoop(){
	bool writeOk = true;
	if (needsWrite) {
		needsWrite = false;
		wireImpl->beginTransmission(address);
		wireImpl->write(toWrite);
		writeOk = wireImpl->endTransmission() == 0;
	}
	
	if(pinsConfiguredRead) {
		wireImpl->requestFrom(address, (uint8_t)1);
		if (wireImpl->available()) {
			lastRead = wireImpl->read();
		}
	}
	return writeOk;
}
#endif

void PCF8574IoAbstraction::attachInterrupt(pinid_t /*pin*/, RawIntHandler intHandler, uint8_t /*mode*/) {
	// if there's an interrupt pin set
	if(interruptPin == 0xff) return;

	internalDigitalIo()->pinDirection(interruptPin, INPUT_PULLUP);
	internalDigitalIo()->attachInterrupt(interruptPin, intHandler, FALLING);
}

BasicIoAbstraction* ioFrom8574(uint8_t addr, uint8_t interruptPin, WireType wireImpl) {
	return new PCF8574IoAbstraction(addr, interruptPin, wireImpl);
}

MCP23017IoAbstraction::MCP23017IoAbstraction(uint8_t address, Mcp23xInterruptMode intMode, pinid_t intPinA, pinid_t intPinB, WireType wireImpl) {
	this->wireImpl = wireImpl;
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

void MCP23017IoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
	if(needsInit) initDevice();

	toggleBitInRegister(IODIR_ADDR, pin, (mode == INPUT || mode == INPUT_PULLUP));
	toggleBitInRegister(GPPU_ADDR, pin, mode == INPUT_PULLUP);

	bitSet(portFlags, (pin < 8) ? READER_PORTA_BIT : READER_PORTB_BIT);
}

void MCP23017IoAbstraction::writeValue(pinid_t pin, uint8_t value) {
	if(needsInit) initDevice();

	bitWrite(toWrite, pin, value);
	bitSet(portFlags, (pin < 8) ? CHANGE_PORTA_BIT : CHANGE_PORTB_BIT);
}

uint8_t MCP23017IoAbstraction::readValue(pinid_t pin) {
	return bitRead(lastRead, pin);
}

uint8_t MCP23017IoAbstraction::readPort(pinid_t pin) {
	return (pin < 8) ? (lastRead & 0xff) : (lastRead >> 8);
}

void MCP23017IoAbstraction::writePort(pinid_t pin, uint8_t value) {
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

#ifdef __MBED__
bool MCP23017IoAbstraction::writeToDevice(uint8_t reg, uint16_t command) {
    I2CLocker locker(wireImpl);
    char data[3];
    data[0] = reg;
	data[1] = (char)command;
	data[2] = (char)(command>>8);
	return wireImpl->write(address, data, sizeof(data)) == 0;
}

bool MCP23017IoAbstraction::writeToDevice8(uint8_t reg, uint8_t command) {
    I2CLocker locker(wireImpl);
    char data[2];
    data[0] = reg;
    data[1] = (char)command;
    return wireImpl->write(address, data, sizeof(data)) == 0;
}

uint16_t MCP23017IoAbstraction::readFromDevice(uint8_t reg) {
    I2CLocker locker(wireImpl);

    wireImpl->write(address, (char*)&reg, 1);
	char data[2];
	wireImpl->read(address, data, sizeof(data));
	// read will get port A first then port B.
	return data[0] | (data[1] << 8);
}

uint8_t MCP23017IoAbstraction::readFromDevice8(uint8_t reg) {
    I2CLocker locker(wireImpl);

    wireImpl->write(reg);

    char data[1];
	wireImpl->read(address, data, (uint8_t)1);
	return data[1];
}
#else
bool MCP23017IoAbstraction::writeToDevice(uint8_t reg, uint16_t command) {
	wireImpl->beginTransmission(address);
	wireImpl->write(reg);

	// write port A then port B.
	wireImpl->write((uint8_t)command);
	wireImpl->write((uint8_t)(command>>8));
	return wireImpl->endTransmission() == 0;
}

bool MCP23017IoAbstraction::writeToDevice8(uint8_t reg, uint8_t command) {
	wireImpl->beginTransmission(address);
	wireImpl->write(reg);

	// write only one port.
	wireImpl->write((uint8_t)command);
	return wireImpl->endTransmission() == 0;
}

uint16_t MCP23017IoAbstraction::readFromDevice(uint8_t reg) {
	wireImpl->beginTransmission(address);
	wireImpl->write(reg);
	wireImpl->endTransmission(false);

	wireImpl->requestFrom(address, (uint8_t)2);
	// read will get port A first then port B.
	uint8_t portA = wireImpl->read();
	uint16_t portB = (wireImpl->read() << 8);
	return portA | portB;
}

uint8_t MCP23017IoAbstraction::readFromDevice8(uint8_t reg) {
	wireImpl->beginTransmission(address);
	wireImpl->write(reg);
	wireImpl->endTransmission(false);

	wireImpl->requestFrom(address, (uint8_t)1);
	return wireImpl->read();
}
#endif

void MCP23017IoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler intHandler, uint8_t mode) {
	// only if there's an interrupt pin set
	if(intPinA == 0xff) return;

	auto  inbuiltIo = internalDigitalIo();
	uint8_t pm = (intMode == ACTIVE_HIGH_OPEN || intMode == ACTIVE_LOW_OPEN) ? INPUT_PULLUP : INPUT;
	uint8_t im = (intMode == ACTIVE_HIGH || intMode == ACTIVE_HIGH_OPEN) ? RISING : FALLING;
	if(intPinB == 0xff) {
        inbuiltIo->pinDirection(intPinA, pm);
        inbuiltIo->attachInterrupt(intPinA, intHandler, im);
	}
	else {
        inbuiltIo->pinDirection(intPinA, pm);
        inbuiltIo->attachInterrupt(intPinA, intHandler, im);
        inbuiltIo->pinDirection(intPinB, pm);
        inbuiltIo->attachInterrupt(intPinB, intHandler, im);
    }

	toggleBitInRegister(GPINTENA_ADDR, pin, true);
	toggleBitInRegister(INTCON_ADDR, pin, mode != CHANGE);
	toggleBitInRegister(DEFVAL_ADDR, pin, mode == FALLING);
}

void MCP23017IoAbstraction::setInvertInputPin(pinid_t pin, bool shouldInvert) {
    toggleBitInRegister(IPOL_ADDR, pin, shouldInvert);
}

IoAbstractionRef ioFrom23017(uint8_t addr, WireType wireImpl) {
	return ioFrom23017IntPerPort(addr, NOT_ENABLED, 0xff, 0xff, wireImpl);
}

IoAbstractionRef ioFrom23017(uint8_t addr, Mcp23xInterruptMode intMode, pinid_t interruptPin, WireType wireImpl) {
	return ioFrom23017IntPerPort(addr, intMode, interruptPin, 0xff, wireImpl);
}

IoAbstractionRef ioFrom23017IntPerPort(uint8_t addr, Mcp23xInterruptMode intMode, pinid_t intPinA, pinid_t intPinB, WireType wireImpl) {
	return new MCP23017IoAbstraction(addr, intMode, intPinA, intPinB, wireImpl);
}