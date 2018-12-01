#line 2 "taskManagerTests.ino"

#include <AUnit.h>
#include <util/atomic.h>
#include "IoAbstraction.h"
#include "MockIoAbstraction.h"

using namespace aunit;

void setup() {
    Serial.begin(115200);
    while(!Serial); // needed for some 32 bit boards.
}

void loop() {
	TestRunner::run();
}

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

int count1 = 0;
int count2 = 0;

void runLoop1() {
    count1++;
}

void runLoop2() {
    count2++;
}

//
// this test only runs on AVR - it sets the timer near to overflow and schedules some tasks
//
test(testClockRollover) {
    count1 = count2 = 0;
	
	// set the clock so that it will roll
	uint32_t oldMillis = millis();
    setMillis(0xfffffe70UL);

	taskManager.scheduleOnce(1, runLoop1, TIME_SECONDS);
	taskManager.scheduleFixedRate(250, runLoop2, TIME_MICROS);
	
	// make sure it's still to wrap.
	assertTrue(millis() > 100000000UL);

    // now run the loop
	dumpTaskTiming();
    unsigned long start = millis();
    while(count1 == 0 && (millis() - start) < 5000) {
        taskManager.yieldForMicros(10000);
    }

	dumpTaskTiming();

    // the one second task should have executed exactly once.
    assertEqual(count1, 1);
    assertMore(count2, 1000);

	// make sure millis has wrapped now.
	assertTrue(millis() < 10000UL);

	// and make sure the microsecond job is still going..	
	int count2Then = count2;
	taskManager.yieldForMicros(10000);
	assertTrue(count2Then != count2);

    // reset the millisecond timer where it was before.
	setMillis(oldMillis);
}
