#line 2 "taskManagerTests.ino"

#include <AUnit.h>
#include "IoAbstraction.h"
#include "MockIoAbstraction.h"

#define MILLIS_ALLOWANCE 2000
#define MICROS_ALLOWANCE 200

using namespace aunit;

void setup() {
    Serial.begin(115200);
    while(!Serial); // needed for some 32 bit boards.
}

void loop() {
	TestRunner::run();
}

// these variables are set during test runs to time and verify tasks are run.
bool scheduled = false;
bool scheduled2ndJob = false;
unsigned long microsStarted = 0, microsExecuted = 0, microsExecuted2ndJob = 0;
int count = 0, count2 = 0;
uint8_t pinNo = 0;

void recordingJob() {
	microsExecuted = micros();
	count++;
	scheduled = true;
}

void recordingJob2() {
	microsExecuted2ndJob = micros();
	count2++;
	scheduled2ndJob = true;
}
void intHandler(uint8_t pin) {
	scheduled = true;
	microsExecuted = micros();
	count++;
	pinNo = pin;
}

class TimingHelpFixture : public TestOnce {
protected:
	void setup() override {
		taskManager.reset();
		scheduled = scheduled2ndJob = false;
		microsExecuted = microsExecuted2ndJob = 0;
		microsStarted = micros();
		count = count2 = 0;
	}

	void assertThatTaskRunsOnTime(uint32_t minExpected, uint32_t allowanceOver) {
		unsigned long start = millis();

		// wait until the task is marked as scheduled.
		while(!scheduled && (millis() - start) < 5000) {
			taskManager.yieldForMicros(1);
		}

		// we must have called the job.
		assertTrue(scheduled);

		// print information to help with diagnostics.
		Serial.print("Scheduled: "); Serial.print(microsStarted);
		Serial.print(" ExecAt: "); Serial.print(microsExecuted);
		unsigned long howLong = microsExecuted - microsStarted;
		Serial.print(" difference "); Serial.print(howLong);
		Serial.print(" - expected "); Serial.println(minExpected);

		// check that it has been executed within the allowable window.
		unsigned long minRange = (minExpected < allowanceOver) ? 0 : (minExpected - allowanceOver);
		unsigned long maxRange = minExpected + allowanceOver;
		assertMoreOrEqual((microsExecuted - microsStarted), minRange);
		assertLess((microsExecuted - microsStarted), maxRange);
	}

	void assertThatSecondJobRan(unsigned long minExpected, unsigned long allowanceOver) {
		// it is assumed that the first job has been waited for and that this should
		// have been run first (ie the shorter task)

		// print information to help with diagnostics.
		Serial.print("Scheduled: "); Serial.print(microsStarted);
		Serial.print(" ExecAt: "); Serial.print(microsExecuted2ndJob);
		long howLong = microsExecuted2ndJob - microsStarted;
		Serial.print(" difference "); Serial.print(howLong);
		Serial.print(" - expected "); Serial.println(minExpected);

		assertTrue(scheduled2ndJob);
		unsigned long minRange = (minExpected < allowanceOver) ? 0 : (minExpected - allowanceOver);
		unsigned long maxRange = minExpected + allowanceOver;
		assertMoreOrEqual((microsExecuted2ndJob - microsStarted), minRange);
		assertLess((microsExecuted2ndJob - microsStarted), maxRange);
	}

	void assertTasksSpacesTaken(int taken) {
		char sz[20];
		int count = 0;
		char* taskData = taskManager.checkAvailableSlots(sz);
		while(*taskData) {
			if(*taskData != 'F') count++;
			++taskData;
		} 
		Serial.print("Tasks free "); Serial.println(count);
		assertEqual(taken, count);
	}
};

testF(TimingHelpFixture, schedulingTaskOnceInMicroseconds) {
	taskManager.scheduleOnce(800, recordingJob, TIME_MICROS);
	assertThatTaskRunsOnTime(800, MICROS_ALLOWANCE);
	assertTasksSpacesTaken(0);
}

testF(TimingHelpFixture, schedulingTaskOnceInMilliseconds) {
	taskManager.scheduleOnce(20, recordingJob, TIME_MILLIS);
	assertThatTaskRunsOnTime(19500, MILLIS_ALLOWANCE);
	assertTasksSpacesTaken(0);
}

testF(TimingHelpFixture, schedulingTaskOnceInSeconds) {
	taskManager.scheduleOnce(2, recordingJob, TIME_SECONDS);
	// second scheduling is not as granular, we need to allow +- 100mS.
	assertThatTaskRunsOnTime(2000000L, MILLIS_ALLOWANCE);
	assertTasksSpacesTaken(0);
}

testF(TimingHelpFixture, scheduleManyJobsAtOnce) {
	taskManager.scheduleOnce(1, [] {}, TIME_SECONDS);
	taskManager.scheduleOnce(200, recordingJob, TIME_MILLIS);
	taskManager.scheduleOnce(250, recordingJob2, TIME_MICROS);

	assertThatTaskRunsOnTime(199500, MILLIS_ALLOWANCE);
	assertThatSecondJobRan(250, MICROS_ALLOWANCE);
	assertTasksSpacesTaken(1);
}

testF(TimingHelpFixture, scheduleFixedRateTestCase) {
	taskManager.scheduleFixedRate(1, recordingJob, TIME_MILLIS);
	taskManager.scheduleFixedRate(100, recordingJob2, TIME_MICROS);

	uint32_t timeStartYield = millis();
	taskManager.yieldForMicros(20000);
	uint32_t timeTaken = millis() - timeStartYield;

	// make sure the yield timings were in range.
	assertLess(timeTaken, (uint32_t) 22);
	assertMoreOrEqual(timeTaken, (uint32_t) 19);

	// now make sure that we got in the right ball park of calls.
	assertMore(count, 15);
	assertMore(count2, 150);
}

testF(TimingHelpFixture, interruptSupportMarshalling) {
	MockedIoAbstraction mockIo;
	taskManager.setInterruptCallback(intHandler);
	taskManager.addInterrupt(&mockIo, 2, CHANGE);

	// make sure the interrupt is properly registered.
	assertTrue(mockIo.isIntRegisteredAs(2, CHANGE));

	// now pretend the interrupt took place.
	(mockIo.getInterruptFunction())();

	// and wait for task manager to schedule.
	assertThatTaskRunsOnTime(0, 250);
	
	// and the pin should be 2
	assertEqual(2, pinNo);
}

testF(TimingHelpFixture, cancellingAJobAfterCreation) {
	int taskId = taskManager.scheduleFixedRate(1, recordingJob, TIME_MILLIS);

	assertThatTaskRunsOnTime(1000, MILLIS_ALLOWANCE);

	assertTasksSpacesTaken(1);

	taskManager.cancelTask(taskId);

	assertTasksSpacesTaken(0);
}

// We can only reset the clock to a new value on AVR, this is very useful and allows us to ensure the
// rollover cases work properly at least for milliseconds

#ifdef __AVR__
#include <util/atomic.h>
void setMillis(unsigned long ms)
{
  extern unsigned long timer0_millis;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    timer0_millis = ms;
  }
}

//
// this test only runs on AVR - it sets the timer near to overflow and schedules some tasks
//
testF(TimingHelpFixture, testClockRollover) {
	
	// set the clock so that it will roll
	uint32_t oldMillis = millis();
    setMillis(((uint32_t)-200L));

	taskManager.scheduleOnce(1, recordingJob, TIME_SECONDS);
	taskManager.scheduleFixedRate(250, recordingJob2, TIME_MICROS);
	
	// make sure it's still to wrap.
	assertTrue(millis() > 100000000L);

	// now make sure it actually runs as expected
	assertThatTaskRunsOnTime(1000000L, MILLIS_ALLOWANCE);
	assertTasksSpacesTaken(1);

	// make sure it has wrapped now.
	assertTrue(millis() < 10000L);

	// and make sure the microsecond job is still going..	
	int count2Then = count2;
	taskManager.yieldForMicros(10000);
	assertTrue(count2Then != count2);

	setMillis(oldMillis);
}

#endif // AVR test only

int counts[6];

void testCall1() {
	counts[0]++;
}

void testCall2() {
	counts[1]++;
}

void testCall3() {
	counts[2]++;
}

void testCall4() {
	counts[3]++;
	taskManager.scheduleOnce(1, testCall4, TIME_SECONDS);
}

void testCall6() {
	counts[5]++;
	if(counts[5] < 10) taskManager.scheduleOnce(500, testCall6);
}

void testCall5() {
	counts[4]++;
	taskManager.scheduleOnce(1000, testCall6);
}

void clearCounts() {
	for(int i=0;i<6;i++) counts[i]=0;
	taskManager.reset();
}

//
// This test actually runs the task manager full up with tasks for around 20 seconds, scheduling using
// lots of different intervals from micros through to seconds, using both single shot and repeating
// schedules, this is the most important test to pass in the whole suite.
//
test(taskManagerHighThroughputTest) {
	char slotData[15];
	clearCounts();

	Serial.print("Dumping thread queue"); Serial.println(taskManager.checkAvailableSlots(slotData));

	taskManager.scheduleFixedRate(10, testCall1);
	taskManager.scheduleFixedRate(100, testCall2);
	taskManager.scheduleFixedRate(100, testCall3, TIME_MICROS);
	taskManager.scheduleOnce(1, testCall4, TIME_SECONDS);
	taskManager.scheduleOnce(10, testCall5, TIME_SECONDS);

	Serial.print("Dumping thread queue"); Serial.println(taskManager.checkAvailableSlots(slotData));

	unsigned long start = millis();
	while(counts[5] < 10 && (millis() - start) < 25000) {
		taskManager.yieldForMicros(10000);
	}

	Serial.print("Dumping thread queue"); Serial.println(taskManager.checkAvailableSlots(slotData));

	assertEqual(counts[5], 10); 	// should be 10 runs as it's manually repeating
	assertMore(counts[0], 1600);	// should be at least 1600 runs it's scheduled every 10 millis
	assertMore(counts[1], 160);		// should be at least 160 runs, scheduled every 100 millis, 
	assertMore(counts[3], 16);		// should be at least 16 runs, as this test lasts about 20 seconds. 
	assertEqual(counts[4], 1); 		// these should both be 1, trigged exactly once.
	assertNotEqual(counts[2], 0); 	// meaningless to count micros calls. check it happened
}

