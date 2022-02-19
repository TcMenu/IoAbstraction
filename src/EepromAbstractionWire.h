/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_
#define IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_

/**
 * @file EepromAbstractionWire.h
 * 
 * Contains the i2c variants of the EepromAbstraction
 */

#include "PlatformDeterminationWire.h"
#include "EepromAbstraction.h"
#include <TaskManager.h>


/** the page size for 32kbit (4KB) roms */
#define PAGESIZE_AT24C32   32
/** the page size for 64kbit (8KB) roms */
#define PAGESIZE_AT24C64   32
/** the page size for 128kbit (16KB) roms */
#define PAGESIZE_AT24C128  64
/** the page size for 256kbit (32KB) roms */
#define PAGESIZE_AT24C256  64
/** the page size for 512bit (64KB) roms */
#define PAGESIZE_AT24C512 128

/**
 * An implementation of eeprom that works with the very well known At24CXXX chips over i2c. Before
 * using this class you must first initialise the Wire library by calling Wire.begin(); If you
 * do not do this, your code may hang. Further, avoid any call to read or write until at least
 * the setup() function is called. This is a limitation of the way the Wire library gets
 * constructed.
 *
 * It is your responsibility to call Wire.begin because you don't want more than one class
 * reinitialising the Wire library.
 *
 * Thanks to https://github.com/cyberp/AT24Cx for some of the ideas I've used in this library,
 * although this is implemented differently.
 */

#ifdef __AVR__
#define WIRE_BUFFER_SIZE 16
#else
#define WIRE_BUFFER_SIZE 32
#endif


class I2cAt24Eeprom : public EepromAbstraction {
	WireType wireImpl;
	uint8_t  eepromAddr;
	uint8_t  pageSize;
	bool     errorOccurred;
public:
	/**
	 * Create an I2C EEPROM object giving it's address and the page size of the device.
	 * Page sizes are defined in this header file.
	 */
    I2cAt24Eeprom(uint8_t address, uint8_t pageSize, WireType wireImpl = defaultWireTypePtr);
	~I2cAt24Eeprom() override = default;

	/** 
	 * This indicates if an I2C error has ocrrued at any point since the last call to error.
	 * Side effect: Every call clears it's state.
	 */
	bool hasErrorOccurred() override;

	uint8_t read8(EepromPosition position) override;
	void write8(EepromPosition position, uint8_t val) override;

	uint16_t read16(EepromPosition position) override;
	void write16(EepromPosition position, uint16_t val) override;

	uint32_t read32(EepromPosition position) override;
	void write32(EepromPosition position, uint32_t val) override;

	void readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) override;
	void writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) override;
private:
	uint8_t findMaximumInPage(uint16_t romDest, uint8_t len) const;
	void writeByte(EepromPosition position, uint8_t val);
	uint8_t readByte(EepromPosition position);
    void writeAddressWire(uint16_t memAddr, const uint8_t* data = nullptr, int len = 0);
};

#endif /* IOABSTRACTION_EEPROMABSTRACTIONWIRE_H_ */
