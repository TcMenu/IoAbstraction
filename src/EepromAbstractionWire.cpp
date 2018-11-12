/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <EepromAbstractionWire.h>
#include <Wire.h>

#define READY_TRIES_COUNT 100

I2cAt24Eeprom::I2cAt24Eeprom(uint8_t address, uint8_t pageSize) {
	this->eepromAddr = address;
	this->pageSize = pageSize;
}

void I2cAt24Eeprom::writeAddressWire(uint16_t memAddr) {
	Wire.write(memAddr >> 8);
	Wire.write(memAddr & 0xff);
}

void I2cAt24Eeprom::waitForReady(uint8_t eeprom) {
	// as per discussion with Koepel, probably a good idea to bound the number of
	// tries here so it does not lock up in the case of hardware problems. 
	// It now tries for a few millis waiting for the rom to write. It will wait
	// around 5 - 10 milliseconds depending on the bus speed.
	uint16_t triesLeft = READY_TRIES_COUNT;
	do {
		// when not on the first time around, introduce a small delay while the eeprom settles.
		// this gives us more certainty that we'll wait long enough before timing out.
		if(triesLeft != READY_TRIES_COUNT) delayMicroseconds(50);
		Wire.beginTransmission(eeprom);
		--triesLeft;
	} while(Wire.endTransmission() != 0 && triesLeft != 0);

	// if we timed out (triesLeft = 0) then we set the error condition.
	errorOccurred |= (triesLeft == 0); 
}

bool I2cAt24Eeprom::hasErrorOccurred() {
	bool ret = errorOccurred;
	errorOccurred = false;
	return ret;
}

uint8_t I2cAt24Eeprom::readByte(EepromPosition position) {
	Wire.beginTransmission(eepromAddr);
	writeAddressWire(position);
	Wire.endTransmission();

	uint8_t ret = 0;
	Wire.requestFrom(eepromAddr, (uint8_t)1, (uint8_t)true);
	if(Wire.available()) ret = (uint8_t)Wire.read();
	return ret;
} 

uint8_t I2cAt24Eeprom::read8(EepromPosition position) {
	waitForReady(eepromAddr);
    return readByte(position);
}

void I2cAt24Eeprom::write8(EepromPosition position, uint8_t val) {
	if(read8(position) == val) return;
	writeByte(position, val);
}

void I2cAt24Eeprom::writeByte(EepromPosition position, uint8_t val) {
	// before ANY write that is going to be committed (by ending i2c send)
	// then we must first wait for the device to be ready. In case a previous
	// write has not yet completed.
	waitForReady(eepromAddr);

	Wire.beginTransmission(eepromAddr);
	writeAddressWire(position);
	Wire.write(val);
	Wire.endTransmission();
}

uint16_t I2cAt24Eeprom::read16(EepromPosition position) {
	waitForReady(eepromAddr);

	uint16_t ret = ((uint16_t)readByte(position++) << 8);
	ret |= readByte(position);
	return ret;
}

void I2cAt24Eeprom::write16(EepromPosition position, uint16_t val) {
	if(read16(position) == val) return;

	writeByte(position++, (uint8_t)(val >> 8));
	writeByte(position,   (uint8_t)val);
}

uint32_t I2cAt24Eeprom::read32(EepromPosition position) {
	waitForReady(eepromAddr);

	uint32_t ret = ((uint32_t)readByte(position++) << 24);
	ret |= ((uint32_t)readByte(position++) << 16);
	ret |= ((uint32_t)readByte(position++) << 8);
	ret |= (readByte(position));
	return ret;
}

void I2cAt24Eeprom::write32(EepromPosition position, uint32_t val) {
	if(read32(position) == val) return;

	writeByte(position++, (uint8_t)(val >> 24));
	writeByte(position++, (uint8_t)(val >> 16));
	writeByte(position++, (uint8_t)(val >> 8));
	writeByte(position,   (uint8_t)val);
}

uint8_t I2cAt24Eeprom::findMaximumInPage(uint16_t destEeprom, uint8_t len) {
	// We can read/write in bulk, but do no exceed the page size or we will read / write
	// the wrong bytes
	int offs = destEeprom % pageSize;
	int currentGo = min((int)pageSize, offs + len) - offs;

	// dont exceed the buffer length of the  wire library
	return min(currentGo, WIRE_BUFFER_SIZE);
}

void I2cAt24Eeprom::readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) {
	int romOffset = 0;
	while(len) {
		waitForReady(eepromAddr);
		uint8_t currentGo = findMaximumInPage(romSrc + romOffset, len);

		Wire.beginTransmission(eepromAddr);
		writeAddressWire(romSrc + romOffset);
		Wire.endTransmission();

		Wire.requestFrom(eepromAddr, (uint8_t)currentGo, (uint8_t)true);
		while(len && Wire.available()) {
			memDest[romOffset] = (uint8_t)Wire.read();
			--len;
			++romOffset;
		}
	}
}

void I2cAt24Eeprom::writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) {
	int romOffset = 0;
	while(len > 0) {
		waitForReady(eepromAddr);
		int currentGo = findMaximumInPage(romDest + romOffset, len);
		Wire.beginTransmission(eepromAddr);
		writeAddressWire(romDest + romOffset);
		while(currentGo) {
			Wire.write(memSrc[romOffset]);
			--currentGo;
			--len;
			++romOffset;
		}
		Wire.endTransmission();
	}
}
