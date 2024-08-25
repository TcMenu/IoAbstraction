#include <Arduino.h>
#include <IoAbstraction.h>
#include <EepromAbstractionWire.h>
#include <TaskManagerIO.h>
#include <unity.h>

/**
 * NOTES:
 *  This is a local only test, and requires `Wire` to be properly configured and an I2C ROM attached. It will check
 *  the ROM operations are working correctly for At24 based EEPROMs.
 *
 *  DO NOT USE UNLESS YOU WANT ALL CONTENTS OF THE ROM CLEARED DOWN AND RESET
 */

char memToWrite[110];
char readBuffer[110];
const uint8_t i2cAddr = 0x50;
const At24EepromType eepromType = PAGESIZE_AT24C16;//PAGESIZE_AT24C128;

const char smallerTestString[] = "Test string that exceeds page size";
const char longerTestString[] = "This is a test string that exceeds page size on larger EEPROMs with big pages";

uint8_t pageSize;
size_t eepromSize;

void testI2CEeprom() {
    int loopsPerformed = 0;

    I2cAt24Eeprom eeprom(i2cAddr, eepromType);
    for(int i=0; i<eepromSize; i++) {
        eeprom.write8(i, 0);
        serlogF2(SER_DEBUG, "Loop ", i)
        TEST_ASSERT_EQUAL(0, eeprom.read8(i));
        TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());
        loopsPerformed++;
    }

    serdebugF2("8 Bit finished OK, loops = ", loopsPerformed);

    for(int i=0; i<(eepromSize - 2); i+=2) {
        eeprom.write16(i, i);
        serlogF2(SER_DEBUG, "Loop ", i)
        TEST_ASSERT_EQUAL((uint16_t)i, eeprom.read16(i));
        TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());
        loopsPerformed++;
    }

    serdebugF2("16 Bit finished OK", loopsPerformed);

    eeprom.write32(eepromSize - 10, 0xf00dbeef);
    auto ret = eeprom.read32(eepromSize - 10);
    TEST_ASSERT_EQUAL(ret, 0xf00dbeef);
    TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());

    const char* dataToWrite = (pageSize < 16) ? smallerTestString : longerTestString;
    size_t sizeToWrite = (pageSize < 16) ? sizeof smallerTestString : sizeof longerTestString;
    auto where = eepromSize - (sizeToWrite + 20);
    eeprom.writeArrayToRom(where, reinterpret_cast<const uint8_t *>(dataToWrite), sizeToWrite);
    TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());

    uint8_t readBack[100];
    eeprom.readIntoMemArray(readBack, where, sizeToWrite);
    TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());

    serdebugF2("Array read back = ", (const char*)readBack);

    TEST_ASSERT_TRUE(strncmp((const char*)readBack, dataToWrite, sizeToWrite) == 0);
    serdebugF("Array test finished OK");

    eeprom.read8(eepromSize * 2);
    TEST_ASSERT_TRUE(eeprom.hasErrorOccurred());
    serdebugF("Oversize finished OK");
}

bool romClear(EepromAbstraction& eeprom, EepromPosition pos) {
    for(int i=0;i<100;i++) {
        eeprom.write8(pos + i, 0xaa);

        if(eeprom.read8(pos + i) != 0xaa) return false;
    }
    return true;
}


void testI2cArrayWrites() {
    I2cAt24Eeprom eeprom(0x50, PAGESIZE_AT24C128);
    TEST_ASSERT_TRUE(romClear(eeprom, 700));
    serdebug("Run array tests on i2c rom");

    strcpy(memToWrite, "This is a very large string to write into the rom to ensure it crosses memory boundaries in the rom");
    eeprom.writeArrayToRom(710, (const uint8_t*)memToWrite, sizeof(memToWrite));

    serdebug("I2C eeprom array written.");

    eeprom.readIntoMemArray((uint8_t*)readBuffer, 710, sizeof(memToWrite));
    TEST_ASSERT_EQUAL_STRING(memToWrite, readBuffer);
    serdebug("Read into mem done");
}

void testI2cSingleWrites() {
    serdebug("Run single tests on i2c rom");
    I2cAt24Eeprom eeprom(0x50, PAGESIZE_AT24C128);
    TEST_ASSERT_TRUE(romClear(eeprom, 700));

    eeprom.write8(700, 0xFF);
    TEST_ASSERT_EQUAL(0xFF, eeprom.read8(700));
    eeprom.write8(700, 0xDD);
    TEST_ASSERT_EQUAL(0xDD, eeprom.read8(700));

    eeprom.write16(701, 0xf00d);
    eeprom.write32(703, 0xbeeff00d);

    yield();
    serdebug("I2C reads...");

    TEST_ASSERT_EQUAL((uint16_t)0xf00d, eeprom.read16(701));
    TEST_ASSERT_EQUAL((uint32_t)0xbeeff00d, eeprom.read32(703));

    TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());
}

void badI2cEepromDoesNotLockCode() {
    serdebug("I2C bad EEPROM address test start.");

    I2cAt24Eeprom eepromBad(0x73, PAGESIZE_AT24C128);
    eepromBad.write8(800, 123);
    TEST_ASSERT_TRUE(eepromBad.hasErrorOccurred());

    serdebug("I2C bad EEPROM address test end.");
}

#ifdef __AVR__
void testClockRollover();
#endif // AVR

void setup() {
    Wire.begin();
    Serial.begin(115200);
    while (!Serial);

    pageSize = at24PageFromRomSize(eepromType);
    eepromSize = at24ActualSizeFromRomSize(eepromType);

    UNITY_BEGIN();

    RUN_TEST(testI2CEeprom);
    RUN_TEST(testI2cSingleWrites);
    RUN_TEST(testI2cArrayWrites);
    RUN_TEST(badI2cEepromDoesNotLockCode);

#ifdef __AVR__
    RUN_TEST(testClockRollover);
#endif // AVR

    UNITY_END();
}

void loop() {

}