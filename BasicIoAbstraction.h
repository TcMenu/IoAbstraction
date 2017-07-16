
#ifndef _BASIC_IO_ABSTRACTION_H_
#define _BASIC_IO_ABSTRACTION_H_

/*
BasicIoAbstraction library, written by thecoderscorner.com
*/


#include <Arduino.h>

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


//
// helpers to create the various type of IO Facilities 
//
BasicIoAbstraction* ioFrom8754(uint8_t addr);
BasicIoAbstraction* ioUsingArduino();

#endif
