
#include <Arduino.h>
#include <unity.h>
#include <MockEepromAbstraction.h>
#include <EepromAbstractionWire.h>
#include "IoAbstraction.h"
#include "negatingIoAbstractionTests.h"
#include "switchesTests.h"
#include "ioDeviceTests.h"
#include "SimpleCollectionsTest.h"

const char memToWrite[110] = { "This is a very large string to write into the rom to ensure it crosses memory boundaries in the rom" };

void testI2cEeepromOnGoodAddress() {
    I2cAt24Eeprom eeprom(0x50, PAGESIZE_AT24C128);
    eeprom.write8(700, 0xfe);
    eeprom.write16(701, 0xf00d);
    eeprom.write32(703, 0xbeeff00d);
    serdebug("I2C eeprom writing array.");

    eeprom.writeArrayToRom(710, (const uint8_t*)memToWrite, sizeof(memToWrite));
    serdebug("I2C eeprom writes complete.");

    char readBuffer[128];
    TEST_ASSERT_EQUAL(0xfe, eeprom.read8(700));
    TEST_ASSERT_EQUAL(0xf00d, eeprom.read16(701));
    TEST_ASSERT_EQUAL(0xbeeff00d, eeprom.read32(703));
    eeprom.readIntoMemArray((uint8_t*)readBuffer, 710, sizeof(memToWrite));
    TEST_ASSERT_EQUAL_STRING(memToWrite, readBuffer);

    // now try other values to ensure the prior test worked
    eeprom.write8(700, 0xaa);
    TEST_ASSERT_EQUAL(0xaa, eeprom.read8(700));
    TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());

    I2cAt24Eeprom eepromBad(0x73, PAGESIZE_AT24C128);
    eepromBad.write8(800, 123);
    TEST_ASSERT_TRUE(eepromBad.hasErrorOccurred());
}

void testMockEeprom() {
    MockEepromAbstraction eeprom(256);
    eeprom.write8(0, 0xfe);
    eeprom.write16(1, 0xf00d);
    eeprom.write32(3, 0xbeeff00d);
    eeprom.writeArrayToRom(10, (const uint8_t*)memToWrite, sizeof(memToWrite));

    char readBuffer[128];
    TEST_ASSERT_EQUAL(0xfe, eeprom.read8(0));
    TEST_ASSERT_EQUAL(0xf00d, eeprom.read16(1));
    TEST_ASSERT_EQUAL(0xbeeff00d, eeprom.read32(3));
    eeprom.readIntoMemArray((uint8_t*)readBuffer, 10, sizeof(memToWrite));
    TEST_ASSERT_EQUAL_STRING(memToWrite, readBuffer);

    // now try other values to ensure the prior test worked
    eeprom.write8(0, 0xaa);
    TEST_ASSERT_EQUAL(0xaa, eeprom.read8(0));
    TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());

    // write beyond boundary
    eeprom.write16(1000, 0xbad);
    TEST_ASSERT_TRUE(eeprom.hasErrorOccurred());
}

void setup() {
    while(!Serial);
    Wire.begin();
    delay(2000);
    UNITY_BEGIN();
    // negating abstraction tests
    RUN_TEST(testNegatingIoAbstractionRead);

    // device abstraction tests.
    RUN_TEST(testMockIoAbstractionRead);
    RUN_TEST(testMockIoAbstractionWrite);
    RUN_TEST(testMultiIoPassThrough);

    // rom tests
    RUN_TEST(testMockEeprom);
    RUN_TEST(testI2cEeepromOnGoodAddress);

    // collection tests
    RUN_TEST(testNearestLocationEdgeCases);
    RUN_TEST(testAddingWithoutSortOrResize);
    RUN_TEST(testAddingWithSortNoResize);
    RUN_TEST(testAddingWithSortAndResizeBy5);

    // switches tests
    RUN_TEST(testPressingASingleButton);
    RUN_TEST(testInterruptButtonRepeating);
    RUN_TEST(testUpDownEncoder);
}

void loop() {
    UNITY_END();
}