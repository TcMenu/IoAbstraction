
/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _BASIC_IO_ABSTRACTION_H_
#define _BASIC_IO_ABSTRACTION_H_

#include <Arduino.h>
/**
 * @file IoAbstaction.h
 * Using basic IoFacilities allows one to abstract away the use of IoExpanders, such 
 * that the switching from BasicIoFacilities to IoExpanderFacilities allows the same
 * code to use an IoExpander instead of direct pins
 */

#include "BasicIoAbstraction.h"

/**
 * An implementation of BasicIoFacilities that supports the ubiquitous shift
 * register, using 74HC165 for input (pins 0 to 23) and a 74HC595 for output (24 onwards).
 */
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
	/** 
	 * Normally use the shift register helper functions to create an instance.
	 * @see inputOutputFromShiftRegister
	 * @see inputOnlyFromShiftRegister
	 * @see outputOnlyFromShiftRegister
	 */
	ShiftRegisterIoAbstraction(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t readClockEnaPin, uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin);
	virtual ~ShiftRegisterIoAbstraction() { }
	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	/**
	 * Interrupts are not supported on shift registers
	 */
	virtual void attachInterrupt(uint8_t, RawIntHandler, uint8_t) {;}
	virtual void runLoop();
	
	/**
	 * writes to the output shift register - currently always port 0
	 */
	virtual void writePort(uint8_t port, uint8_t portVal);

	/**
	 * reads from the input shift register - currently always port 3
	 */
	virtual uint8_t readPort(uint8_t port);
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

	/** 
	 * delegates the pin direction call to whichever abstraction owns the pin, and that
	 * abstraction will then set the pin mode
	 * @param pin the pin to set the mode on
	 * @param mode as per pinMode modes
	 */
	virtual void pinDirection(uint8_t pin, uint8_t mode);

	/**
	 * delegates writing the value to whichever abstraction owns the pin, this abstraction
	 * will then write out the value.
	 * @param pin the pin to write to
	 * @param value the value to write to the pin
	 */
	virtual void writeValue(uint8_t pin, uint8_t value);

	/**
	 * delegates reading the value from a pin to whichever abstraction owns the pin.
	 * @param pin the pin to read from 
	 */
	virtual uint8_t readValue(uint8_t pin);

	/**
	 * delegates writing the port value to the abstraction that owns that pin, the abstraction
	 * will then determine the port that the pin belongs to
	 * @param port any pin within the port
	 * @param portVal an 8 bit value to write direct to the port. 
	 */
	virtual void writePort(uint8_t port, uint8_t portVal);

	/**
	 * delegates reading a port to the abstraction that owns the pin, the abstraction will then
	 * determine which port owns the pin and return the port value.
	 * @param port any pin within that port.
	 */
	virtual uint8_t readPort(uint8_t port);

	/**
	 * delegates attaching an interrupt to the abstraction that owns the pin, see each abstraction
	 * for more information about how interrupts work with the given device.
	 * @param pin the pin on the device
	 * @param intHandler the interrupt intHandler
	 * @param mode as per arduino interrupt modes.
	 */
	virtual void attachInterrupt(uint8_t pin, RawIntHandler intHandler, uint8_t mode);

	/**
	 * will run through all delegate abstractions and sync them
	 */
	virtual void runLoop();
private:
	uint8_t doExpanderOp(uint8_t pin, uint8_t aVal, ExpanderOpFn fn);
};

/**
 * performs both input and output functions using two shift registers, one for reading and one for writing.  As shift registers have a fixed direction
 * input and output are handled by different devices, and therefore fixed at the time of building the circuit. This abstraction works as follows:
 * 
 * * Input pins of the input shift register start at 0
 * * Output pins of the output shift register start at 24.
 * 
 * @param readClockPin the clock pin on the INPUT shift register
 * @param readDataPin the data pin on the INPUT shift register
 * @param readLatchPin the latch pin on the INPUT shift register
 * @param readClockEnaPin the clock enable pin on the INPUT shift register.
 * @param writeClockPin the clock pin on the OUTPUT shift register
 * @param writeDataPin the data pin on the OUTPUT shift register
 * @param writeLatchPin the latch pin on the OUTPUT shift register
 */
IoAbstractionRef inputOutputFromShiftRegister(uint8_t readClockPin, uint8_t readDataPin, uint8_t readLatchPin, uint8_t readClockEnaPin, uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin);

/**
 * performs input only functions using a shift register, the input pins of the shift register start at pin 0.
 * @param readClockPin the clock pin on the INPUT shift register
 * @param dataPin the data pin on the INPUT shift register
 * @param latchPin the latch pin on the INPUT shift register
 * @param readClockEnaPin the clock enable pin on the INPUT shift register.
 */
IoAbstractionRef inputOnlyFromShiftRegister(uint8_t readClockPin, uint8_t readClockEnaPin, uint8_t dataPin, uint8_t latchPin);

/**
 * performs output only functions using a shift register, the output pins of the shift register start at 24.
 * @param writeClockPin the clock pin on the OUTPUT shift register
 * @param writeDataPin the data pin on the OUTPUT shift register
 * @param writeLatchPin the latch pin on the OUTPUT shift register
 */
IoAbstractionRef outputOnlyFromShiftRegister(uint8_t writeClockPin, uint8_t writeDataPin, uint8_t writeLatchPin);

#include "TaskManager.h"
#include "SwitchInput.h"

#endif
