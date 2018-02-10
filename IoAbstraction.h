
#ifndef _BASIC_IO_ABSTRACTION_H_
#define _BASIC_IO_ABSTRACTION_H_

#include <Arduino.h>

/*
BasicIoAbstraction library, Apache 2.0 licnense, written by thecoderscorner.com
*/

// Using basic IoFacilities allows one to abstract away the use of IoExpanders, such 
// that the switching from BasicIoFacilities to IoExpanderFacilities allows the same
// code to use an IoExpander instead of direct pins

class BasicIoAbstraction {
public:
	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	virtual void runLoop() { ; }
};


// An implementation of BasicIoFacilties that supports the PCF8574 i2c IO chip.
class PCF8574IoAbstraction : public BasicIoAbstraction {
private:
	uint8_t address;
	uint8_t lastRead;
	uint8_t toWrite;
	bool needsWrite;
public:
	PCF8574IoAbstraction(uint8_t addr);
	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	virtual void runLoop();
private:
	void writeData();
	uint8_t readData();
};

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
	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	virtual void runLoop();
};

//
// helpers to create the various type of IO Facilities 
//

// Help with usage of the library without resorting pointers

typedef BasicIoAbstraction* IoAbstractionRef;

inline void ioDevicePinMode(IoAbstractionRef ioDev, uint8_t pin, uint8_t dir) { ioDev->pinDirection(pin, dir); }
inline uint8_t ioDeviceDigitalRead(IoAbstractionRef ioDev, uint8_t pin) { return ioDev->readValue(pin); }
inline void ioDeviceDigitalWrite(IoAbstractionRef ioDev, uint8_t pin, uint8_t val) { ioDev->writeValue(pin, (val)); }
inline void ioDeviceSync(IoAbstractionRef ioDev) { ioDev->runLoop(); }

/*
 * passes calls to digitalRead and write directly through to arduino pins.
 */
IoAbstractionRef ioUsingArduino();

/*
 * performs digital read and write function using an 8574 IO expander chip
 */
IoAbstractionRef ioFrom8754(uint8_t addr);

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

#include "TaskManager.h"
#include "SwitchInput.h"

#endif
