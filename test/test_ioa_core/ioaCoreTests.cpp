
#include <IoAbstraction.h>
#include <PlatformDeterminationWire.h>
#include <MockEepromAbstraction.h>
#include <EepromAbstractionWire.h>
#include <unity.h>


char memToWrite[110] = { };
char readBuffer[110] = { };

void testMockEeprom() {
    MockEepromAbstraction eeprom(256);
    eeprom.write8(0, 0xfe);
    eeprom.write16(1, 0xf00d);
    eeprom.write32(3, 0xbeeff00d);
    eeprom.writeArrayToRom(10, (const uint8_t*)memToWrite, sizeof(memToWrite));

    TEST_ASSERT_EQUAL((uint8_t)0xfe, eeprom.read8(0));
    TEST_ASSERT_EQUAL((uint16_t)0xf00d, eeprom.read16(1));
    TEST_ASSERT_EQUAL((uint32_t)0xbeeff00d, eeprom.read32(3));
    eeprom.readIntoMemArray((uint8_t*)readBuffer, 10, sizeof(memToWrite));
    TEST_ASSERT_EQUAL_STRING(memToWrite, readBuffer);

    // now try other values to ensure the prior test worked
    eeprom.write8(0, 0xaa);
    TEST_ASSERT_EQUAL((uint8_t)0xaa, eeprom.read8(0));
    TEST_ASSERT_FALSE(eeprom.hasErrorOccurred());

    // write beyond boundary
    eeprom.write16(1000, 0xbad);
    TEST_ASSERT_TRUE(eeprom.hasErrorOccurred());
}
