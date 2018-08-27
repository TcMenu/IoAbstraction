/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "IoAbstraction.h"

#define LATCH_TIME 5

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
	this->lastRead = 0;

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

void ShiftRegisterIoAbstraction::pinDirection(__attribute((unused)) uint8_t pin, __attribute((unused)) uint8_t mode) {
	// ignored, this implementation has hardwired inputs and outputs - inputs are 0-23, outputs are 24 onwards
}

void ShiftRegisterIoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	if (pin < 24) return;
	pin = pin - 24;

	toWrite = bitWrite(toWrite, pin, value);
	needsWrite = true;
}

void ShiftRegisterIoAbstraction::writePort(uint8_t pin, uint8_t portVal) {
	if(pin < 24) return;
	toWrite = portVal;
	needsWrite = true;
}

uint8_t ShiftRegisterIoAbstraction::readPort(uint8_t pin) {
	return lastRead;
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

MultiIoAbstraction::MultiIoAbstraction(uint8_t arduinoPinsNeeded) {
	limits[0] = arduinoPinsNeeded;
	delegates[0] = ioUsingArduino();
	numDelegates = 1;
}

MultiIoAbstraction::~MultiIoAbstraction() {
	// delegates added are our responsibility to clean up
	for(uint8_t i=0; i<numDelegates; ++i) {
		delete delegates[i];
	}
}

void MultiIoAbstraction::addIoExpander(IoAbstractionRef expander, uint8_t numOfPinsNeeded) {
	limits[numDelegates]= limits[numDelegates - 1] + numOfPinsNeeded;
	delegates[numDelegates] = expander;

	numDelegates++;
}

uint8_t MultiIoAbstraction::doExpanderOp(uint8_t pin, uint8_t aVal, ExpanderOpFn fn) {
	uint8_t ret = -1;
	for(uint8_t i=0; i<numDelegates; ++i) {
		// when we are on the first expander, the "previous" last pin is 0.
		uint8_t last = (i==0) ? 0 : limits[i-1];

		// then we find the limit of the expander we are on
		uint8_t currLimit = limits[i];

		// and check if we have a match!
		if(pin >= last && pin < currLimit) {
			ret = fn(delegates[i], pin - last, aVal);
			break;
		}
	}
	return ret;
}

void MultiIoAbstraction::pinDirection(uint8_t pin, uint8_t mode) {
	doExpanderOp(pin, mode, [](IoAbstractionRef a, uint8_t p, uint8_t v) {
		a->pinDirection(p, v);
		return (uint8_t)0;
	});
}

void MultiIoAbstraction::writeValue(uint8_t pin, uint8_t value) {
	doExpanderOp(pin, value, [](IoAbstractionRef a, uint8_t p, uint8_t v) {
		a->writeValue(p, v);
		return (uint8_t)0;
	});
}

uint8_t MultiIoAbstraction::readValue(uint8_t pin) {
	return doExpanderOp(pin, 0, [](IoAbstractionRef a, uint8_t p, uint8_t) {
		uint8_t retn = a->readValue(p);
		return retn;
	});
}

void MultiIoAbstraction::writePort(uint8_t pin, uint8_t val) {
	doExpanderOp(pin, val, [](IoAbstractionRef a, uint8_t p, uint8_t v) {
		a->writePort(p, v);
		return (uint8_t)0;
	});
}

uint8_t MultiIoAbstraction::readPort(uint8_t pin) {
	return doExpanderOp(pin, 0, [](IoAbstractionRef a, uint8_t p, uint8_t v) {
		return a->readPort(p);
	});
}

void MultiIoAbstraction::attachInterrupt(uint8_t pin, RawIntHandler intHandler, uint8_t mode) {
	for(uint8_t i=0; i<numDelegates; ++i) {
		// when we are on the first expander, the "previous" last pin is 0.
		uint8_t last = (i==0) ? 0 : limits[i-1];

		// then we find the limit of the expander we are on
		uint8_t currLimit = limits[i];

		// and check if we have a match!
		if(pin >= last && pin < currLimit) {
			delegates[i]->attachInterrupt(pin - last, intHandler, mode);
			break;
		}
	}
}

void MultiIoAbstraction::runLoop() {
	for(uint8_t i=0; i<numDelegates; ++i) {
		delegates[i]->runLoop();
	}
}

// helper functions to create the abstractions. 

IoAbstractionRef outputOnlyFromShiftRegister(uint8_t writeClkPin, uint8_t dataPin, uint8_t latchPin) {
	return new ShiftRegisterIoAbstraction(0xff, 0xff, 0xff, 0xff, writeClkPin, dataPin, latchPin);
}

IoAbstractionRef inputOnlyFromShiftRegister(uint8_t readClkPin, uint8_t readClkEnaPin, uint8_t dataPin, uint8_t latchPin) {
	return new ShiftRegisterIoAbstraction(readClkPin, dataPin, latchPin, readClkEnaPin, 0xff, 0xff, 0xff);
}

IoAbstractionRef inputOutputFromShiftRegister(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t readClockEnaPin, uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin) {
	return new ShiftRegisterIoAbstraction(readClockPin, readDataPin, readLatchPin, readClockEnaPin, writeClockPin, writeDataPin, writeLatchPin);
}

IoAbstractionRef ioUsingArduino() { 
	return new BasicIoAbstraction(); 
}
