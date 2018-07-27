/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <EepromAbstractionWire.h>

uint8_t I2cAt24Eeprom::read8(EepromPosition position) {
	return rom->read(position);
}

void I2cAt24Eeprom::write8(EepromPosition position, uint8_t val) {
	rom->write(position, val);
}

uint16_t I2cAt24Eeprom::read16(EepromPosition position) {
	return rom->readInt(position);
}

void I2cAt24Eeprom::write16(EepromPosition position, uint16_t val) {
	rom->writeInt(position, val);
}

uint32_t I2cAt24Eeprom::read32(EepromPosition position) {
	return rom->readLong(position);
}

void I2cAt24Eeprom::write32(EepromPosition position, uint32_t val) {
	rom->writeLong(position, val);
}

void I2cAt24Eeprom::readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) {
	rom->read(romSrc, memDest, len);
}

void I2cAt24Eeprom::writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) {
	byte* mem = (byte*) memSrc;
	rom->write(romDest, mem, len);
}
