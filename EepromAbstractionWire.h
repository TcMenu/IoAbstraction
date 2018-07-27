/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */


#ifndef _IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_
#define _IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_

#include <Arduino.h>
#include "EepromAbstraction.h"
#include "AT24CX.h"
#include<Wire.h>

//
// If you don't want to use task manager to do the yield when waiting for the i2c eeprom, then set
// then comment out the line below, so it's not defined..
//
#define YIELD_USING_TASK_MGR true

/**
 * An implementation of eeprom that works with the very well known At24C256 chip over i2c. Construct
 * this class passing in the i2c address on which the chip can be found.
 *
 * Requires https://github.com/cyberp/AT24Cx library to work.
 */
class I2cAt24Eeprom : public EepromAbstraction {
	AT24CX* rom;
public:
	I2cAt24Eeprom(AT24CX* rom) { this->rom = rom; }
	virtual ~I2cAt24Eeprom() {}

	virtual uint8_t read8(EepromPosition position);
	virtual void write8(EepromPosition position, uint8_t val);

	virtual uint16_t read16(EepromPosition position);
	virtual void write16(EepromPosition position, uint16_t val);

	virtual uint32_t read32(EepromPosition position);
	virtual void write32(EepromPosition position, uint32_t val);

	virtual void readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len);
	virtual void writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len);
};

#endif /* _IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_ */
