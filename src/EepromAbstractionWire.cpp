/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <EepromAbstractionWire.h>
#include <IoLogging.h>
#include <TaskManager.h>

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

uint8_t I2cAt24Eeprom::findMaximumInPage(uint16_t destEeprom, uint8_t len) {
	// We can read/write in bulk, but do no exceed the page size or we will read / write
	// the wrong bytes
	uint16_t offs = destEeprom % pageSize;
    uint16_t currentGo = min((uint16_t)pageSize, uint16_t(offs + len)) - offs;

	// dont exceed the buffer length of the  wire library
	return min(currentGo, (uint16_t)WIRE_BUFFER_SIZE);
}

uint8_t I2cAt24Eeprom::read8(EepromPosition position) {
    waitForReady();
    return readByte(position);
}

void I2cAt24Eeprom::write8(EepromPosition position, uint8_t val) {
    if(read8(position) == val) return;
    writeByte(position, val);
}

uint16_t I2cAt24Eeprom::read16(EepromPosition position) {
    waitForReady();

    uint16_t ret = ((uint16_t)readByte(position++) << 8U);
    ret |= readByte(position);
    return ret;
}

void I2cAt24Eeprom::write16(EepromPosition position, uint16_t val) {
    if(read16(position) == val) return;

    auto hiByte = (uint8_t)(val >> 8U);
    auto loByte = (uint8_t)val;
    writeByte(position, hiByte);
    writeByte(position + 1,   loByte);
}

uint32_t I2cAt24Eeprom::read32(EepromPosition position) {
    waitForReady();

    uint32_t ret = ((uint32_t)readByte(position++)) << 24U;
    ret |= ((uint32_t)readByte(position++)) << 16U;
    ret |= ((uint32_t)readByte(position++)) << 8U;
    ret |= (readByte(position));
    return ret;
}

void I2cAt24Eeprom::write32(EepromPosition position, uint32_t val) {
    if(read32(position) == val) return;

    writeByte(position++, (uint8_t)(val >> 24U));
    writeByte(position++, (uint8_t)(val >> 16U));
    writeByte(position++, (uint8_t)(val >> 8U));
    writeByte(position,   (uint8_t)val);
}

#ifdef __MBED__

#define checkForError(flag, ret) (flag = (flag || ((ret) !=0)))

void I2cAt24Eeprom::waitForReady() {
    // not used on mbed, integrated into the read operation
}

uint8_t I2cAt24Eeprom::readByte(EepromPosition position) {
    I2CLocker locker(wireImpl);

    writeAddressWire(position);
    char data[1];
    checkForError(errorOccurred, wireImpl->read(eepromAddr, data, sizeof(data)));

    return (uint8_t)*data;
}

void I2cAt24Eeprom::writeByte(EepromPosition position, uint8_t val) {
    I2CLocker locker(wireImpl);
    char data[1];
    data[0] = (char)val;
    writeAddressWire(position, data, 1);
    checkForError(errorOccurred, wireImpl->write(eepromAddr, data, sizeof(data)));
}

void I2cAt24Eeprom::writeAddressWire(EepromPosition memAddr, const char *data, int len) {
    if(len > 32)
    {
        errorOccurred = true;
        return;
    }
    uint8_t ch[34];
    ch[0] = memAddr >> 8U;
    ch[1] = memAddr & 0xffU;
    int tries = 0;
    if(data != nullptr) memcpy(ch + 2, data, len);
    while(tries < READY_TRIES_COUNT && wireImpl->write(eepromAddr, (const char*)ch, len + 2, false) !=0) {
        taskManager.yieldForMicros(50);
        tries++;
    }
    errorOccurred = tries == READY_TRIES_COUNT;
}

void I2cAt24Eeprom::readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) {
    I2CLocker locker(wireImpl);
    int romOffset = 0;
    while(len > 0 && !errorOccurred) {
        int currentGo = findMaximumInPage(romSrc + romOffset, len);

        writeAddressWire(romSrc + romOffset);
        checkForError(errorOccurred, wireImpl->read(eepromAddr, (char*)&memDest[romOffset], currentGo));
        romOffset += currentGo;
        len -= currentGo;
    }
}

void I2cAt24Eeprom::writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) {
    I2CLocker locker(wireImpl);
    int romOffset = 0;
    while(len > 0 && !errorOccurred) {
        int currentGo = findMaximumInPage(romDest + romOffset, len);
        writeAddressWire(romDest + romOffset, (char*)&memSrc[romOffset], currentGo);
        len -= currentGo;
        romOffset += currentGo;
    }
}

#else


uint8_t I2cAt24Eeprom::readByte(EepromPosition position) {
	wireImpl->beginTransmission(eepromAddr);
	writeAddressWire(position);
	wireImpl->endTransmission();

	uint8_t ret = 0;
	wireImpl->requestFrom(eepromAddr, (uint8_t)1);
	if(wireImpl->available()) ret = (uint8_t)wireImpl->read();
	return ret;
}

void I2cAt24Eeprom::writeByte(EepromPosition position, uint8_t val) {
	// before ANY write that is going to be committed (by ending i2c send)
	// then we must first wait for the device to be ready. In case a previous
	// write has not yet completed.
	waitForReady();

	wireImpl->beginTransmission(eepromAddr);
	writeAddressWire(position);
	wireImpl->write(val);
	wireImpl->endTransmission();
}

void I2cAt24Eeprom::readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) {
	int romOffset = 0;
	while(len) {
		waitForReady();
		uint8_t currentGo = findMaximumInPage(romSrc + romOffset, len);

		wireImpl->beginTransmission(eepromAddr);
		writeAddressWire(romSrc + romOffset);
		wireImpl->endTransmission();

		wireImpl->requestFrom(eepromAddr, (uint8_t)currentGo);
		while(len && wireImpl->available()) {
			memDest[romOffset] = (uint8_t)wireImpl->read();
			--len;
			++romOffset;
		}
	}
}
void I2cAt24Eeprom::writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) {
	int romOffset = 0;
	while(len > 0) {
		waitForReady();
		int currentGo = findMaximumInPage(romDest + romOffset, len);
		wireImpl->beginTransmission(eepromAddr);
		writeAddressWire(romDest + romOffset);
		while(currentGo) {
			wireImpl->write(memSrc[romOffset]);
			--currentGo;
			--len;
			++romOffset;
		}
		wireImpl->endTransmission();
	}
}

void I2cAt24Eeprom::writeAddressWire(uint16_t memAddr) {
	wireImpl->write(memAddr >> 8);
	wireImpl->write(memAddr & 0xff);
}

void I2cAt24Eeprom::waitForReady() {
	// as per discussion with Koepel, probably a good idea to bound the number of
	// tries here so it does not lock up in the case of hardware problems.
	// It now tries for a few millis waiting for the rom to write. It will wait
	// around 5 - 10 milliseconds depending on the bus speed.
	uint16_t triesLeft = READY_TRIES_COUNT;
	do {
		// when not on the first time around, introduce a small delay while the eeprom settles.
		// this gives us more certainty that we'll wait long enough before timing out.
		if(triesLeft != READY_TRIES_COUNT) taskManager.yieldForMicros(50);
		wireImpl->beginTransmission(eepromAddr);
		--triesLeft;
	} while(wireImpl->endTransmission() != 0 && triesLeft != 0);

	// if we timed out (triesLeft = 0) then we set the error condition.
	if(triesLeft == 0) {
        errorOccurred = true;
        serdebugF("EEPROM: Out of retries");
    }
}

#endif