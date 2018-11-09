#ifndef _MOCK_EEPROM_ABSTRACTION_H_
#define _MOCK_EEPROM_ABSTRACTION_H_

#include "EepromAbstraction.h"

#define EEPROM_MOCK_SIZE 64;

class MockEepromAbstraction : public EepromAbstraction {
private:
	bool errorFlag;
	uint8_t data[EEPROM_MOCK_SIZE];
public:
	virtual ~MockEepromAbstraction() {}

	bool hasErrorOccurred() override { return errorFlag;}
	void clearError() {errorFlag = false;}

	void checkBounds(EepromPosition pos, int len) {
		if(pos + len >= EEPROM_MOCK_SIZE) {
			errorFlag = true;
		}
	}

	uint8_t read8(EepromPosition position) override {
		checkBounds(position, 1);
		return data[position];
	}

	void write8(EepromPosition position, uint8_t val) override {
		checkBounds(position, 1);
		data[position] = val;
	}

	uint16_t read16(EepromPosition position) override {
		return read8(position) | (read8(position + 1) << 8);
	}

	void write16(EepromPosition position, uint16_t val) override {
		write8(position, val);
		write8(position + 1, val >> 8);
	}

	virtual uint32_t read32(EepromPosition position) override {
		checkBounds(position, 4);
		return (uint32_t)read8(position) | ((uint32_t)read8(position + 1) << 8) | ((uint32_t)read8(position + 1) << 16) | ((uint32_t)read8(position + 1) << 24);
	}

	virtual void write32(EepromPosition position, uint32_t val) override {
		checkBounds(position, 4);
		write8(position, val);
		write8(position + 1, val >> 8);
		write8(position + 2, val >> 16);
		write8(position + 3, val >> 24);
	}

	virtual void readIntoMemArray(uint8_t* memDest, EepromPosition romSrc, uint8_t len) override {
		checkBounds(position, len);
		memcpy(memDest, romSrc, len);
	}

	virtual void writeArrayToRom(EepromPosition romDest, const uint8_t* memSrc, uint8_t len) override {
		checkBounds();
		memcpy(romDest, memSrc, len);
	}
};

#endif
