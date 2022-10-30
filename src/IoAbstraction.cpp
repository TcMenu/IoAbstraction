/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "PlatformDetermination.h"
#include "IoAbstraction.h"

#define LATCH_TIME 5

#ifdef IOA_USE_MBED
#include <mbed.h>
#define digitalWriteS(x, y) ioDeviceDigitalWriteS(internalDigitalIo(), x, y)
#define digitalReadS(x) ioDeviceDigitalReadS(internalDigitalIo(), x)
#define pinModeS(x, y) ioDevicePinMode(internalDigitalIo(), x, y)

uint8_t shiftIn(pinid_t dataPin, pinid_t clockPin, ShiftBitOrder bitOrder) {
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i) {
        digitalWriteS(clockPin, HIGH);
        if(bitOrder == LSBFIRST) {
            value |= digitalReadS(dataPin) << i;
        } else {
            value |= digitalReadS(dataPin) << (7 - i);
        }
        digitalWriteS(clockPin, HIGH);
        digitalWriteS(clockPin, LOW);
    }
    return value;
}

void shiftOut(pinid_t dataPin, pinid_t clockPin, ShiftBitOrder bitOrder, uint8_t val) {
    uint8_t i;

    for(i = 0; i < 8; i++) {
        if(bitOrder == LSBFIRST) {
            digitalWriteS(dataPin, !!(val & (1 << i)));
        } else {
            digitalWriteS(dataPin, !!(val & (1 << (7 - i))));
        }

        digitalWriteS(clockPin, HIGH);
        digitalWriteS(clockPin, LOW);
    }
}
#else
#include <Arduino.h>
#define digitalWriteS(x, y) digitalWrite(x, y)
#define digitalReadS(x) digitalRead(x)
#define pinModeS(x, y) pinMode(x, y)
#endif


ShiftRegisterIoAbstraction::ShiftRegisterIoAbstraction(pinid_t readClockPin, pinid_t readDataPin, pinid_t readLatchPin, pinid_t writeClockPin, pinid_t writeDataPin,
                                                       pinid_t writeLatchPin, uint8_t noReadDevices, uint8_t noWriteDevices) {
	needsWrite = true;
	toWrite = 0;

	this->readClockPin = readClockPin;
	this->readDataPin = readDataPin;
	this->readLatchPin = readLatchPin;
	this->writeLatchPin = writeLatchPin;
	this->writeDataPin = writeDataPin;
	this->writeClockPin = writeClockPin;
	this->lastRead = 0;
	this->numOfDevicesRead = noReadDevices;
	this->numOfDevicesWrite = noWriteDevices; 

	if (writeDataPin != 0xff) {
		pinModeS(writeLatchPin, OUTPUT);
		pinModeS(writeDataPin, OUTPUT);
		pinModeS(writeClockPin, OUTPUT);
		digitalWriteS(writeLatchPin, LOW);
	}

	if (readLatchPin != 0xff) {
		pinModeS(readLatchPin, OUTPUT);
		pinModeS(readDataPin, INPUT);
		pinModeS(readClockPin, OUTPUT);
		digitalWriteS(readLatchPin, HIGH);
	}
}

void ShiftRegisterIoAbstraction::pinDirection(__attribute((unused)) pinid_t pin, __attribute((unused)) uint8_t mode) {
	// ignored, this implementation has hardwired inputs and outputs - inputs are 0-31, outputs are 32 onwards
}

void ShiftRegisterIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
	if (pin < SHIFT_REGISTER_OUTPUT_CUTOVER) return;
	pin = pin - SHIFT_REGISTER_OUTPUT_CUTOVER;

	bitWrite(toWrite, pin, value);
	needsWrite = true;
}

void ShiftRegisterIoAbstraction::writePort(pinid_t pin, uint8_t portV) {
	uint32_t portVal = portV;
	if(pin < SHIFT_REGISTER_OUTPUT_CUTOVER) return;
	pin = pin - SHIFT_REGISTER_OUTPUT_CUTOVER;
	if(pin < 8) {
		toWrite &= 0xffffff00;
		toWrite |= portVal;
	}
	else if(pin < 16) {
		toWrite &= 0xffff00ff;
		toWrite |= (portVal << 8);
	}
	else if(pin < 24) {
		toWrite &= 0xff00ffff;
		toWrite |= (portVal << 16);
	}
	else {
		toWrite &= 0x00ffffff;
		toWrite |= (portVal << 24);
	}
	needsWrite = true;
}

uint8_t ShiftRegisterIoAbstraction::readPort(pinid_t pin) {
	if(pin < 8) {
		return lastRead & 0xff;
	}
	else if(pin < 16) {
		return (lastRead >> 8) & 0xff;
	}
	else if(pin < 24) {
		return (lastRead >> 16) & 0xff;
	}
	else {
		return (lastRead >> 24) & 0xff;
	}
}

uint8_t ShiftRegisterIoAbstraction::readValue(pinid_t pin) {
	return ((lastRead & (1 << pin)) != 0) ? HIGH : LOW;
}

bool ShiftRegisterIoAbstraction::runLoop() {
	uint8_t i;
	if (readDataPin != 0xff) {
		digitalWriteS(readLatchPin, LOW);
		taskManager.yieldForMicros(LATCH_TIME);
		digitalWriteS(readLatchPin, HIGH);

		lastRead = 0;
		for(i = 0; i < numOfDevicesRead; ++i) {
			lastRead = lastRead << 8;
			lastRead |= (shiftIn(readDataPin, readClockPin, MSBFIRST) & 0xff);
		}
	}
	
	if (writeDataPin != 0xff && needsWrite) {
		digitalWriteS(writeLatchPin, LOW);
		taskManager.yieldForMicros(LATCH_TIME);
		
		uint32_t shiftLocal = toWrite;
		for(i = 0; i < numOfDevicesWrite; ++i) {
			uint8_t regVal = (shiftLocal & 0xff);
			shiftOut(writeDataPin, writeClockPin, MSBFIRST, regVal);
			shiftLocal = shiftLocal >> 8;
		}
		needsWrite = false;
		digitalWriteS(writeLatchPin, HIGH);
	}
	return true;
}

// helper functions to create the abstractions.

IoAbstractionRef outputOnlyFromShiftRegister(uint8_t writeClkPin, uint8_t dataPin, uint8_t latchPin, uint8_t numOfDevices) {
    return new ShiftRegisterIoAbstraction(0xff, 0xff, 0xff, writeClkPin, dataPin, latchPin, 1, numOfDevices);
}

IoAbstractionRef inputOnlyFromShiftRegister(uint8_t readClkPin, uint8_t dataPin, uint8_t latchPin, uint8_t numOfDevices) {
    return new ShiftRegisterIoAbstraction(readClkPin, dataPin, latchPin, 0xff, 0xff, 0xff, numOfDevices, 1);
}

IoAbstractionRef inputOutputFromShiftRegister(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t numOfReadDevices,
                                              uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin, uint8_t numOfWriteDevices) {
    return new ShiftRegisterIoAbstraction(readClockPin, readDataPin, readLatchPin, writeClockPin, writeDataPin, writeLatchPin, numOfReadDevices, numOfWriteDevices);
}

IoAbstractionRef inputOutputFromShiftRegister(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin,
                                              uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin) {
    return new ShiftRegisterIoAbstraction(readClockPin, readDataPin, readLatchPin, writeClockPin, writeDataPin, writeLatchPin, 1, 1);
}


ShiftRegisterIoAbstraction165In::ShiftRegisterIoAbstraction165In(pinid_t readClockPin, pinid_t readDataPin,
                                                                 pinid_t readLatchPin, pinid_t numRead) {
    this->readClockPin = readClockPin;
    this->readDataPin = readDataPin;
    this->readLatchPin = readLatchPin;
    this->lastRead = 0;
    this->numOfDevicesRead = numRead;

    pinModeS(readLatchPin, OUTPUT);
    pinModeS(readDataPin, INPUT);
    pinModeS(readClockPin, OUTPUT);
    digitalWriteS(readLatchPin, HIGH);
}


uint8_t ShiftRegisterIoAbstraction165In::readPort(pinid_t pin) {
    if(pin < 8) {
        return lastRead & 0xff;
    }
    else if(pin < 16) {
        return (lastRead >> 8) & 0xff;
    }
    else if(pin < 24) {
        return (lastRead >> 16) & 0xff;
    }
    else {
        return (lastRead >> 24) & 0xff;
    }
}

uint8_t ShiftRegisterIoAbstraction165In::readValue(pinid_t pin) {
    return ((lastRead & (1 << pin)) != 0) ? HIGH : LOW;
}

bool ShiftRegisterIoAbstraction165In::runLoop() {
    uint8_t i;
    digitalWriteS(readLatchPin, LOW);
    taskManager.yieldForMicros(LATCH_TIME);
    digitalWriteS(readLatchPin, HIGH);

    lastRead = 0;
    for(i = 0; i < numOfDevicesRead; ++i) {
        lastRead = lastRead << 8;
        lastRead |= (shiftInFor165() & 0xff);
    }

    return true;
}

uint8_t ShiftRegisterIoAbstraction165In::shiftInFor165() const {
    uint8_t value = 0;

    for (int8_t i = 7; i >= 0; --i) {
        digitalWriteS(readClockPin, LOW);
        value |= (digitalReadS(readDataPin) << i);
        digitalWriteS(readClockPin, HIGH);
    }
    return value;
}

IoAbstractionRef inputFrom74HC165ShiftRegister(pinid_t readClkPin, pinid_t dataPin, pinid_t latchPin, pinid_t numOfDevices) {
    return new ShiftRegisterIoAbstraction165In(readClkPin, dataPin, latchPin, numOfDevices);
}

MultiIoAbstraction::MultiIoAbstraction(pinid_t arduinoPinsNeeded) {
	limits[0] = arduinoPinsNeeded;
	delegates[0] = internalDigitalIo();
	numDelegates = 1;
}

MultiIoAbstraction::~MultiIoAbstraction() {
	// delegates added are our responsibility to clean up
	for(uint8_t i=0; i<numDelegates; ++i) {
		delete delegates[i];
	}
}

void MultiIoAbstraction::addIoExpander(IoAbstractionRef expander, pinid_t numOfPinsNeeded) {
	limits[numDelegates]= limits[numDelegates - 1] + numOfPinsNeeded;
	delegates[numDelegates] = expander;

	numDelegates++;
}

uint8_t MultiIoAbstraction::doExpanderOp(pinid_t pin, uint8_t aVal, ExpanderOpFn fn) {
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

void MultiIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
	doExpanderOp(pin, mode, [](IoAbstractionRef a, uint8_t p, uint8_t v) {
		a->pinDirection(p, v);
		return (uint8_t)0;
	});
}

void MultiIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
	doExpanderOp(pin, value, [](IoAbstractionRef a, uint8_t p, uint8_t v) {
		a->writeValue(p, v);
		return (uint8_t)0;
	});
}

uint8_t MultiIoAbstraction::readValue(pinid_t pin) {
	return doExpanderOp(pin, 0, [](IoAbstractionRef a, uint8_t p, uint8_t) {
		uint8_t retn = a->readValue(p);
		return retn;
	});
}

void MultiIoAbstraction::writePort(pinid_t pin, uint8_t val) {
	doExpanderOp(pin, val, [](IoAbstractionRef a, uint8_t p, uint8_t v) {
		a->writePort(p, v);
		return (uint8_t)0;
	});
}

uint8_t MultiIoAbstraction::readPort(pinid_t pin) {
	return doExpanderOp(pin, 0, [](IoAbstractionRef a, uint8_t p, uint8_t) {
		return a->readPort(p);
	});
}

void MultiIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler intHandler, uint8_t mode) {
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

bool MultiIoAbstraction::runLoop() {
	bool runStatus = true;
	for(uint8_t i=0; i<numDelegates; ++i) {
		runStatus = runStatus && delegates[i]->runLoop();
	}
	return runStatus;
}
