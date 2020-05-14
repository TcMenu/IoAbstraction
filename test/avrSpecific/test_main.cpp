
#include <unity.h>
#include <util/atomic.h>
#include <EepromAbstraction.h>
#include "IoAbstraction.h"
#include "MockIoAbstraction.h"

// We can only reset the clock to a new value on AVR, this is very useful and allows us to ensure the
// rollover cases work properly at least for milliseconds. As millisecond and microsecond logic are very
// similar it gives some degree of confidence that it's working properly.
//
// Keep this test on it's own in this package. It messes around with the millisecond counter.

#ifdef __AVR__

void setMillis(uint32_t ms)
{
  extern unsigned long timer0_millis;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    timer0_millis = ms;
  }
}

void dumpTaskTiming() {
    Serial.println("Showing task timings");
    TimerTask* task = taskManager.getFirstTask();
    while(task) {
        Serial.print(" - Timing "); Serial.println(task->microsFromNow());
        task = task->getNext();
    }
}

int avrCount1 = 0;
int avrCount2 = 0;

void runLoop1() {
    avrCount1++;
}

void runLoop2() {
    avrCount2++;
}

//
// this test only runs on AVR - it sets the timer near to overflow and schedules some tasks
//

void testAvrClockWrappingCase() {
    avrCount1 = avrCount2 = 0;
	
	// set the clock so that it will roll
	uint32_t oldMillis = millis();
    setMillis(0xfffffe70UL);

	taskManager.scheduleOnce(1, runLoop1, TIME_SECONDS);
	taskManager.scheduleFixedRate(250, runLoop2, TIME_MICROS);
	
	// make sure it's still to wrap.
    TEST_ASSERT_TRUE(millis() > 100000000UL);

    // now run the loop
	dumpTaskTiming();
    unsigned long start = millis();
    while(avrCount1 == 0 && (millis() - start) < 5000) {
        taskManager.yieldForMicros(10000);
    }

	dumpTaskTiming();

    // the one second task should have executed exactly once.
    TEST_ASSERT_EQUAL(1, avrCount1);
    TEST_ASSERT_GREATER_OR_EQUAL(1000, avrCount2);

	// make sure millis has wrapped now.
	TEST_ASSERT_TRUE(millis() < 10000UL);

	// and make sure the microsecond job is still going..	
	int avrCount2Then = avrCount2;
	taskManager.yieldForMicros(10000);
	TEST_ASSERT_TRUE(avrCount2Then != avrCount2);

    // reset the millisecond timer where it was before.
	setMillis(oldMillis);
}

const char * memToWrite = "This is a very large string to write into the rom to ensure it crosses memory boundaries in the rom";

void avrEepromWrapperTestCase() {
    AvrEeprom eeprom;
    eeprom.write8(700, 0xfe);
    eeprom.write16(701, 0xf00d);
    eeprom.write32(703, 0xbeeff00d);
    auto memLen = strlen(memToWrite) + 1;
    eeprom.writeArrayToRom(710, (const uint8_t*)memToWrite, memLen);

    char readBuffer[128];
    TEST_ASSERT_EQUAL(0xfe, eeprom.read8(700));
    TEST_ASSERT_EQUAL(0xf00d, eeprom.read16(701));
    TEST_ASSERT_EQUAL(0xbeeff00d, eeprom.read32(703));
    eeprom.readIntoMemArray((uint8_t*)readBuffer, 710, memLen);
    TEST_ASSERT_EQUAL_STRING(memToWrite, readBuffer);

    // now try other values to ensure the prior test worked
    eeprom.write8(700, 0xaa);
    TEST_ASSERT_EQUAL(0xaa, eeprom.read8(700));
}

void eepromClassWrapperTestCase() {
    AvrEeprom eeprom;
    eeprom.write8(720, 0xfe);
    eeprom.write16(721, 0xf00d);
    eeprom.write32(723, 0xbeeff00d);
    auto memLen = strlen(memToWrite) + 1;
    eeprom.writeArrayToRom(730, (const uint8_t*)memToWrite, memLen);

    char readBuffer[128];
    TEST_ASSERT_EQUAL(0xfe, eeprom.read8(720));
    TEST_ASSERT_EQUAL(0xf00d, eeprom.read16(721));
    TEST_ASSERT_EQUAL(0xbeeff00d, eeprom.read32(723));
    eeprom.readIntoMemArray((uint8_t*)readBuffer, 730, memLen);
    TEST_ASSERT_EQUAL_STRING(memToWrite, readBuffer);

    // now try other values to ensure the prior test worked
    eeprom.write8(720, 0xaa);
    TEST_ASSERT_EQUAL(0xaa, eeprom.read8(720));
}

void setup() {
    delay(2000);
    UNITY_BEGIN();    // IMPORTANT LINE!
    RUN_TEST(testAvrClockWrappingCase);
    RUN_TEST(avrEepromWrapperTestCase);
    RUN_TEST(eepromClassWrapperTestCase);
}

#else // not __AVR__

void setup() {
    delay(2000);
    UNITY_BEGIN();    // IMPORTANT LINE!

    TEST_FAIL_MESSAGE("Only supported on AVR");
}

#endif // __AVR__

void loop() {
    UNITY_END();
}