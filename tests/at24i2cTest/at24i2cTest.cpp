#include <Arduino.h>
#include <IoAbstraction.h>
#include <EepromAbstractionWire.h>
#include <TaskManagerIO.h>
#include <testing/SimpleTest.h>

using namespace SimpleTest;

/**
 * This test will clear all data on the rom in question, DO NOT USE UNLESS YOU WANT ALL CONTENTS CLEARED DOWN AND RESET
 */

const uint8_t i2cAddr = 0x50;
const At24EepromType eepromType = PAGESIZE_AT24C16;//PAGESIZE_AT24C128;

const char smallerTestString[] = "Test string that exceeds page size";
const char longerTestString[] = "This is a test string that exceeds page size on larger EEPROMs with big pages";

uint8_t pageSize;
size_t eepromSize;

void setup() {
    Wire.begin();
    Serial.begin(115200);
    while (!Serial);

    pageSize = at24PageFromRomSize(eepromType);
    eepromSize = at24ActualSizeFromRomSize(eepromType);

    startTesting();

}

test(testI2CEeprom) {
    int loopsPerformed = 0;

    I2cAt24Eeprom eeprom(i2cAddr, eepromType);
    for(int i=0; i<eepromSize; i++) {
        eeprom.write8(i, 0);
        serlogF2(SER_DEBUG, "Loop ", i)
        assertEquals(0, eeprom.read8(i));
        assertFalse(eeprom.hasErrorOccurred());
        loopsPerformed++;
    }

    serdebugF2("8 Bit finished OK, loops = ", loopsPerformed);

    for(int i=0; i<(eepromSize - 2); i+=2) {
        eeprom.write16(i, i);
        serlogF2(SER_DEBUG, "Loop ", i)
        assertEquals((uint16_t)i, eeprom.read16(i));
        assertFalse(eeprom.hasErrorOccurred());
        loopsPerformed++;
    }

    serdebugF2("16 Bit finished OK", loopsPerformed);

    eeprom.write32(eepromSize - 10, 0xf00dbeef);
    auto ret = eeprom.read32(eepromSize - 10);
    assertEquals(ret, 0xf00dbeef);
    assertFalse(eeprom.hasErrorOccurred());

    const char* dataToWrite = (pageSize < 16) ? smallerTestString : longerTestString;
    size_t sizeToWrite = (pageSize < 16) ? sizeof smallerTestString : sizeof longerTestString;
    auto where = eepromSize - (sizeToWrite + 20);
    eeprom.writeArrayToRom(where, reinterpret_cast<const uint8_t *>(dataToWrite), sizeToWrite);
    assertFalse(eeprom.hasErrorOccurred());

    uint8_t readBack[100];
    eeprom.readIntoMemArray(readBack, where, sizeToWrite);
    assertFalse(eeprom.hasErrorOccurred());

    serdebugF2("Array read back = ", (const char*)readBack);

    assertTrue(strncmp((const char*)readBack, dataToWrite, sizeToWrite) == 0);
    serdebugF("Array test finished OK");

    eeprom.read8(eepromSize * 2);
    assertTrue(eeprom.hasErrorOccurred());
    serdebugF("Oversize finished OK");
}

DEFAULT_TEST_RUNLOOP
