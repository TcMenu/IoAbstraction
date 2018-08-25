
/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _BASIC_IO_ABSTRACTION_H_
#define _BASIC_IO_ABSTRACTION_H_

#include <Arduino.h>

// Using basic IoFacilities allows one to abstract away the use of IoExpanders, such 
// that the switching from BasicIoFacilities to IoExpanderFacilities allows the same
// code to use an IoExpander instead of direct pins

#include "ioAbstractionCoreTypes.h"

// An implementation of BasicIoFacilities that supports the ubiquitous shift
// register, using one for input (pins 0-7) and one for output (8-15).
class ShiftRegisterIoAbstraction : public BasicIoAbstraction {
private:
	uint8_t toWrite;
	uint8_t lastRead;
	bool needsWrite;

	uint8_t readDataPin;
	uint8_t writeDataPin;
	uint8_t readLatchPin;
	uint8_t readClkEnablePin;
	uint8_t writeLatchPin;
	uint8_t readClockPin, writeClockPin;
public:
	ShiftRegisterIoAbstraction(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t readClockEnaPin, uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin);
	virtual ~ShiftRegisterIoAbstraction() { }
	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	virtual void attachInterrupt(uint8_t pin, RawIntHandler intHandler, uint8_t mode) {;}
	virtual void runLoop();
};

// this defines the number of IOExpanders can be put into a multi IO expander.
#define MAX_ALLOWABLE_DELEGATES 8

typedef uint8_t (*ExpanderOpFn)(IoAbstractionRef ref, uint8_t pin, uint8_t val);

// An implementation of the BasicIoAbstraction that provides support for more than one IOExpander
// in a single abstraction, along with a single set of Arduino pins.
// Arduino pins will be from 0..arduinoPinsNeeded in the constructor
// expands will directly follow this, expanders are added using addIoExpander
class MultiIoAbstraction : public BasicIoAbstraction {
private:
	IoAbstractionRef delegates[MAX_ALLOWABLE_DELEGATES];
	uint8_t limits[MAX_ALLOWABLE_DELEGATES];
	uint8_t numDelegates;
public:
	MultiIoAbstraction(uint8_t arduinoPinsNeeded = 100);
	virtual ~MultiIoAbstraction();
	void addIoExpander(IoAbstractionRef expander, uint8_t numOfPinsNeeded);

	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	virtual void attachInterrupt(uint8_t pin, RawIntHandler intHandler, uint8_t mode);
	virtual void runLoop();
private:
	uint8_t doExpanderOp(uint8_t pin, uint8_t aVal, ExpanderOpFn fn);
};

//
// helpers to create the various type of IO Facilities 
//

inline void ioDevicePinMode(IoAbstractionRef ioDev, uint8_t pin, uint8_t dir) { ioDev->pinDirection(pin, dir); }
inline uint8_t ioDeviceDigitalRead(IoAbstractionRef ioDev, uint8_t pin) { return ioDev->readValue(pin); }
inline void ioDeviceDigitalWrite(IoAbstractionRef ioDev, uint8_t pin, uint8_t val) { ioDev->writeValue(pin, (val)); }
inline void ioDeviceSync(IoAbstractionRef ioDev) { ioDev->runLoop(); }
inline void ioDeviceAttachInterrupt(IoAbstractionRef ioDev, uint8_t pin, RawIntHandler intHandler, uint8_t mode) {ioDev->attachInterrupt(pin, intHandler, mode) ;}

//
// Special versions of read and write that also include a sync operation for simple cases
//
inline uint8_t ioDeviceDigitalReadS(IoAbstractionRef ioDev, uint8_t pin) { ioDev->runLoop(); return ioDev->readValue(pin); }
inline void ioDeviceDigitalWriteS(IoAbstractionRef ioDev, uint8_t pin, uint8_t val) { ioDev->writeValue(pin, (val)); ioDev->runLoop(); }

/*
 * passes calls to digitalRead and write directly through to arduino pins.
 */
IoAbstractionRef ioUsingArduino();

/*
* performs both input and output functions using two shift registers, one for reading and one for writing. 
* Input pins of the input shift register show as 0-7.
* Output pins of the output shift register show as 8-15.
*/
IoAbstractionRef inputOutputFromShiftRegister(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t readClockEnaPin, uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin);

/*
* performs input only functions using a shift register, the input pins of the shift register show as 0-7.
*/
IoAbstractionRef inputOnlyFromShiftRegister(uint8_t readClkPin, uint8_t readClkEnaPin, uint8_t dataPin, uint8_t latchPin);

/*
 * performs output only functions using a shift register, the ouyput pins of the shift register show as 8-15.
 */
IoAbstractionRef outputOnlyFromShiftRegister(uint8_t writeClkPin, uint8_t dataPin, uint8_t latchPin);

uint8_t writeBits(uint8_t pin, uint8_t value, uint8_t existingValue);

#include "TaskManager.h"
#include "SwitchInput.h"

#endif
