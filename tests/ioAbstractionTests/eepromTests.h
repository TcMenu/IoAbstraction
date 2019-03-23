#line 2 "eepromTests.h"

// This test requires an At24C eeprom to be installed at 0x50, setup for a 128kbit unit. 
// Also this test needs to run on AVR as it checks the AVR eeprom code too.
// Any changes to the eeprom code require this test to be run before release.

#include <EepromAbstraction.h>
#include <EepromAbstractionWire.h>
#include <MockEepromAbstraction.h>

const char strData[128] = { "this is a really long string that has to be written to eeprom and read back without losing anything at all in the process!"};

class EepromFixtures : public TestOnce {
protected:
	EepromAbstraction *eeprom;
	int romStart = 600;
public:
	void setEeprom(EepromAbstraction* eeprom) {this->eeprom = eeprom;}

	void clearRom() {
		for(EepromPosition pos = 0; pos < 100;pos++) {
			eeprom->write8(romStart + pos, 0);
		}
		Serial.println("Cleared ROM ready for test ");
	}
	
	void standardRomChecks() {
		eeprom->write8(romStart, (byte)42);
		eeprom->write16(romStart + 1, 0xface);
		eeprom->write32(romStart + 3, 0xf00dface);

		assertEqual((uint8_t)42, eeprom->read8(romStart));
		assertEqual((uint16_t) 0xface, eeprom->read16(romStart + 1));
		assertEqual((uint32_t) 0xf00dface, eeprom->read32(romStart + 3));
	}

	void arrayRomChecks() {
		eeprom->writeArrayToRom(romStart + 7, (const unsigned char*)strData, sizeof strData);
		char readBuffer[128];
		eeprom->readIntoMemArray((unsigned char*)readBuffer, romStart + 7, sizeof readBuffer);
		assertEqual(readBuffer, strData);
	}
};

#ifdef __AVR__

testF(EepromFixtures, testAvrEeprom) {
	AvrEeprom anEeprom;
	romStart = 500;
	setEeprom(&anEeprom);
	Serial.println("Doing AVR checks");

	clearRom();	
	standardRomChecks();
	arrayRomChecks();
	assertFalse(anEeprom.hasErrorOccurred());
}

#endif

testF(EepromFixtures, testI2cEeprom) {
	I2cAt24Eeprom anEeprom(0x50, PAGESIZE_AT24C128);
	romStart = 500;
	setEeprom(&anEeprom);
	Serial.println("Doing AT24C128 checks");

	clearRom();	
	standardRomChecks();
	arrayRomChecks();

	assertFalse(anEeprom.hasErrorOccurred());
}

const char helloText[6] = {"Hello"};

testF(EepromFixtures, testTheMockEeeprom) {
	romStart = 0;
	MockEepromAbstraction mockRom;
	setEeprom(&mockRom);
	Serial.println("Doing Mock EEPROM checks");

	clearRom();
	standardRomChecks();

	mockRom.writeArrayToRom(romStart + 7, (const unsigned char*)helloText, sizeof helloText);
	char readBuffer[6];
	mockRom.readIntoMemArray((unsigned char*)readBuffer, romStart + 7, 6);
	assertEqual(readBuffer, helloText);

	assertFalse(mockRom.hasErrorOccurred());
}
