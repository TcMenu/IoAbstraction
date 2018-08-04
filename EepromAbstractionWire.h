/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */


#ifndef _IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_
#define _IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_

#include <Arduino.h>
#include "EepromAbstraction.h"
#include<Wire.h>

#define PAGESIZE_AT24C32   32
#define PAGESIZE_AT24C64   32
#define PAGESIZE_AT24C128  64
#define PAGESIZE_AT24C256  64
#define PAGESIZE_AT24C512 128

/**
 * An implementation of eeprom that works with the very well known At24C256 chip over i2c. Before
 * using this class you must first initialise the Wire library by calling Wire.begin(); If you
 * do not do this, your code may hang. Further, avoid any call to read or write until at least
 * the setup() function is called.
 *
 * It is your responsibility to call Wire.begin because you don't want more than one class
 * reinitialising the Wire library.
 *
 * Thanks to https://github.com/cyberp/AT24Cx for some of the ideas I've used in this library,
 * although this is implemented differently.
 */
class I2cAt24Eeprom : public EepromAbstraction {
	uint8_t eepromAddr;
	uint8_t pageSize;
public:
	/**
	 * Create an I2C EEPROM object giving it's address and the page size of the device.
	 * Page sizes are defined in this header file.
	 */
	I2cAt24Eeprom(uint8_t address, uint8_t pageSize);
	virtual ~I2cAt24Eeprom() {}

	virtual uint8_t read8(EepromPosition position);
	virtual void write8(EepromPosition position, uint8_t val);

	virtual uint16_t read16(EepromPosition position);
	virtual void write16(EepromPosition position, uint16_t val);

	virtual uint32_t read32(EepromPosition position);
	virtual void write32(EepromPosition position, uint32_t val);

	virtual void readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len);
	virtual void writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len);
private:
	uint8_t findMaximumInPage(uint16_t romDest, uint8_t len);
	void writeByte(EepromPosition position, uint8_t val);
};

#endif /* _IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_ */
