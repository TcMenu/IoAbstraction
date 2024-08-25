
#include <TaskManagerIO.h>
#include <unity.h>

#if defined(__AVR__)

#include <util/atomic.h>
#include "IoAbstraction.h"
#include "MockIoAbstraction.h"

// We can only reset the clock to a new value on AVR, this is very useful and allows us to ensure the
// rollover cases work properly at least for milliseconds. As millisecond and microsecond logic are very
// similar it gives some degree of confidence that it's working properly.
//
// Keep this test on it's own in this package. It messes around with the millisecond counter.

void setMillis(unsigned long ms)
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

//
// this test only runs on AVR - it sets the timer near to overflow and schedules some tasks
//
void testClockRollover() {
    avrCount1 = avrCount2 = 0;

    // set the clock so that it will roll
    uint32_t oldMillis = millis();
    setMillis(0xfffffe70UL);

    taskManager.scheduleOnce(1, [] {
        avrCount1++;
    }, TIME_SECONDS);

    taskManager.scheduleFixedRate(250, [] {
        avrCount2++;
    }, TIME_MICROS);

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
    TEST_ASSERT_EQUAL(avrCount1, 1);
    assertMoreThan(1000, avrCount2);

    // make sure millis has wrapped now.
    TEST_ASSERT_TRUE(millis() < 10000UL);

    // and make sure the microsecond job is still going..
    int avrCount2Then = avrCount2;
    taskManager.yieldForMicros(10000);
    TEST_ASSERT_TRUE(avrCount2Then != avrCount2);

    // reset the millisecond timer where it was before.
    setMillis(oldMillis);
}

#endif // __AVR__
