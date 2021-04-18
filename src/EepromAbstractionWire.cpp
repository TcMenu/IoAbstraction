/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "PlatformDetermination.h"
#include <EepromAbstractionWire.h>

#define READY_TRIES_COUNT 100

I2cAt24Eeprom::I2cAt24Eeprom(uint8_t address, uint8_t pageSize, WireType wireImpl) {
	this->wireImpl = wireImpl;
	this->eepromAddr = address;
	this->pageSize = pageSize;
    this->errorOccurred = false;
}

bool I2cAt24Eeprom::hasErrorOccurred() {
    bool ret = errorOccurred;
    errorOccurred = false;
    return ret;
}

uint8_t I2cAt24Eeprom::findMaximumInPage(uint16_t destEeprom, uint8_t len) const {
	// We can read/write in bulk, but do no exceed the page size or we will read / write
	// the wrong bytes
	uint16_t offs = destEeprom % pageSize;
    uint16_t currentGo = min((uint16_t)pageSize, uint16_t(offs + len)) - offs;

	// dont exceed the buffer length of the  wire library
	return min(currentGo, (uint16_t)WIRE_BUFFER_SIZE);
}

uint8_t I2cAt24Eeprom::read8(EepromPosition position) {
    return readByte(position);
}

void I2cAt24Eeprom::write8(EepromPosition position, uint8_t val) {
    TaskMgrLock locker(i2cLock);
    if(read8(position) == val) return;
    writeByte(position, val);
}

uint16_t I2cAt24Eeprom::read16(EepromPosition position) {
    TaskMgrLock locker(i2cLock);
    uint16_t ret = ((uint16_t)readByte(position++) << 8U);
    ret |= readByte(position);
    return ret;
}

void I2cAt24Eeprom::write16(EepromPosition position, uint16_t val) {
    TaskMgrLock locker(i2cLock);
    if(read16(position) == val) return;

    auto hiByte = (uint8_t)(val >> 8U);
    auto loByte = (uint8_t)val;
    writeByte(position, hiByte);
    writeByte(position + 1,   loByte);
}

uint32_t I2cAt24Eeprom::read32(EepromPosition position) {
    TaskMgrLock locker(i2cLock);

    uint32_t ret = ((uint32_t)readByte(position++)) << 24U;
    ret |= ((uint32_t)readByte(position++)) << 16U;
    ret |= ((uint32_t)readByte(position++)) << 8U;
    ret |= (readByte(position));
    return ret;
}

void I2cAt24Eeprom::write32(EepromPosition position, uint32_t val) {
    TaskMgrLock locker(i2cLock);
    if(read32(position) == val) return;

    writeByte(position++, (uint8_t)(val >> 24U));
    writeByte(position++, (uint8_t)(val >> 16U));
    writeByte(position++, (uint8_t)(val >> 8U));
    writeByte(position,   (uint8_t)val);
}

uint8_t I2cAt24Eeprom::readByte(EepromPosition position) {
    TaskMgrLock locker(i2cLock);

    writeAddressWire(position);
    uint8_t data[1];
    errorOccurred = errorOccurred || !ioaWireRead(wireImpl, eepromAddr, data, sizeof(data));

    return (uint8_t)*data;
}

void I2cAt24Eeprom::writeByte(EepromPosition position, uint8_t val) {
    TaskMgrLock locker(i2cLock);
    uint8_t data[1];
    data[0] = (char)val;
    writeAddressWire(position, data, 1);
    errorOccurred = errorOccurred || !ioaWireWriteWithRetry(wireImpl, eepromAddr, data, sizeof(data));
}

void I2cAt24Eeprom::writeAddressWire(EepromPosition memAddr, const uint8_t *data, int len) {
    if(len > 32)
    {
        errorOccurred = true;
        return;
    }
    uint8_t ch[34];
    ch[0] = memAddr >> 8U;
    ch[1] = memAddr & 0xffU;
    if(data != nullptr) memcpy(ch + 2, data, len);
    errorOccurred = errorOccurred || !ioaWireWriteWithRetry(wireImpl, eepromAddr, ch, len + 2, READY_TRIES_COUNT);
}

void I2cAt24Eeprom::readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) {
    TaskMgrLock locker(i2cLock);
    int romOffset = 0;
    while(len > 0 && !errorOccurred) {
        int currentGo = findMaximumInPage(romSrc + romOffset, len);

        writeAddressWire(romSrc + romOffset);
        errorOccurred = errorOccurred || !ioaWireRead(wireImpl, eepromAddr, &memDest[romOffset], currentGo);
        romOffset += currentGo;
        len -= currentGo;
    }
}

void I2cAt24Eeprom::writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) {
    TaskMgrLock locker(i2cLock);
    int romOffset = 0;
    while(len > 0 && !errorOccurred) {
        int currentGo = findMaximumInPage(romDest + romOffset, len);
        if(currentGo > 14) currentGo -= 2; // if we are getting close to potential page size, reduce slightly
        writeAddressWire(romDest + romOffset, &memSrc[romOffset], currentGo);
        len -= currentGo;
        romOffset += currentGo;
    }
}
