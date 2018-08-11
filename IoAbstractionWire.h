/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _IOABSTRACTION_IOABSTRACTIONWIRE_H_
#define _IOABSTRACTION_IOABSTRACTIONWIRE_H_

#include <IoAbstraction.h>

/**
 *  An implementation of BasicIoFacilties that supports the PCF8574 i2c IO chip.
 */
class PCF8574IoAbstraction : public BasicIoAbstraction {
private:
	uint8_t address;
	uint8_t lastRead;
	uint8_t toWrite;
	bool needsWrite;
public:
	PCF8574IoAbstraction(uint8_t addr);
	virtual ~PCF8574IoAbstraction() { }
	virtual void pinDirection(uint8_t pin, uint8_t mode);
	virtual void writeValue(uint8_t pin, uint8_t value);
	virtual uint8_t readValue(uint8_t pin);
	virtual void runLoop();
private:
	void writeData();
	uint8_t readData();
};

/*
 * performs digital read and write function using an 8574 IO expander chip
 */
IoAbstractionRef ioFrom8754(uint8_t addr);

#endif /* _IOABSTRACTION_IOABSTRACTIONWIRE_H_ */
