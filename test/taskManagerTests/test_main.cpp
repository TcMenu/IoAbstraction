
#include <Arduino.h>
#include <unity.h>
#include "IoAbstraction.h"
#include "MockIoAbstraction.h"

#define MILLIS_ALLOWANCE 2000
#define MICROS_ALLOWANCE 200

void dumpTasks() {
	Serial.println("Dumping the task queue contents");
	TimerTask* task = taskManager.getFirstTask();
	while(task) {
		Serial.print(" - Task schedule "); Serial.print(task->microsFromNow());
		Serial.print(task->isRepeating() ? " Repeating ":" Once ");
		Serial.print(task->isJobInMicros() ? " Micros " : task->isJobInSeconds() ? " Seconds " : " Millis ");
		Serial.println(task->isInUse() ? " InUse":" Free");
		if(task->getNext() == task) {
			Serial.println("!!!Infinite loop found!!!");
		}
		task = task->getNext();
	}
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

void prepareTesting() {
    taskManager.reset();
    scheduled = scheduled2ndJob = false;
    microsExecuted = microsExecuted2ndJob = 0;
    microsStarted = micros();
    count = count2 = 0;
}

void assertThatTaskRunsOnTime(uint32_t minExpected, uint32_t allowanceOver) {
    unsigned long startTime = millis();

    // wait until the task is marked as scheduled.
    while(!scheduled && (millis() - startTime) < 5000) {
        taskManager.yieldForMicros(10000);
    }

    // we must have called the job.
    TEST_ASSERT_TRUE(scheduled);

    // print information to help with diagnostics.
    Serial.print("Scheduled: "); Serial.print(microsStarted);
    Serial.print(" ExecAt: "); Serial.print(microsExecuted);
    unsigned long howLong = microsExecuted - microsStarted;
    Serial.print(" difference "); Serial.print(howLong);
    Serial.print(" - expected "); Serial.println(minExpected);

    // check that it has been executed within the allowable window.
    unsigned long minRange = (minExpected < allowanceOver) ? 0 : (minExpected - allowanceOver);
    unsigned long maxRange = minExpected + allowanceOver;
    TEST_ASSERT_GREATER_OR_EQUAL(minRange, (microsExecuted - microsStarted));
    TEST_ASSERT_LESS_OR_EQUAL(maxRange, (microsExecuted - microsStarted));
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

    TEST_ASSERT_TRUE(scheduled2ndJob);
    unsigned long minRange = (minExpected < allowanceOver) ? 0 : (minExpected - allowanceOver);
    unsigned long maxRange = minExpected + allowanceOver;
    TEST_ASSERT_GREATER_OR_EQUAL(minRange, (microsExecuted2ndJob - microsStarted));
    TEST_ASSERT_LESS_OR_EQUAL(maxRange, (microsExecuted2ndJob - microsStarted));
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
    TEST_ASSERT_EQUAL(taken, count);
}

class TestingExec : public Executable {
public:
	int noOfTimesRun;

	TestingExec() {
		noOfTimesRun = 0;
	}

	void exec() override {
		noOfTimesRun++;
	}
};

TestingExec exec;

void testRunningUsingExecutorClass() {
    prepareTesting();

    taskManager.scheduleFixedRate(10, &exec);
	taskManager.scheduleOnce(250, recordingJob);
	assertThatTaskRunsOnTime(250000L, MILLIS_ALLOWANCE);
	TEST_ASSERT_GREATER_OR_EQUAL(10, exec.noOfTimesRun);
}

void testSchedulingTaskOnceInMicroseconds() {
    prepareTesting();

    taskManager.scheduleOnce(800, recordingJob, TIME_MICROS);
	assertThatTaskRunsOnTime(800, MICROS_ALLOWANCE);
	assertTasksSpacesTaken(0);
}

void testSchedulingTaskOnceInMilliseconds() {
    prepareTesting();

    taskManager.scheduleOnce(20, recordingJob, TIME_MILLIS);
	assertThatTaskRunsOnTime(19500, MILLIS_ALLOWANCE);
	assertTasksSpacesTaken(0);
}

void testSchedulingTaskOnceInSeconds() {
    prepareTesting();

    taskManager.scheduleOnce(2, recordingJob, TIME_SECONDS);
	// second scheduling is not as granular, we need to allow +- 100mS.
	assertThatTaskRunsOnTime(2000000L, MILLIS_ALLOWANCE);
	assertTasksSpacesTaken(0);
}

void testScheduleManyJobsAtOnce() {
    prepareTesting();

    taskManager.scheduleOnce(1, [] {}, TIME_SECONDS);
	taskManager.scheduleOnce(200, recordingJob, TIME_MILLIS);
	taskManager.scheduleOnce(250, recordingJob2, TIME_MICROS);

	assertThatTaskRunsOnTime(199500, MILLIS_ALLOWANCE);
	assertThatSecondJobRan(250, MICROS_ALLOWANCE);
	assertTasksSpacesTaken(1);
}

void testScheduleFixedRateTestCase() {
    prepareTesting();

    TEST_ASSERT_NULL(taskManager.getFirstTask());

	int taskId1 = taskManager.scheduleFixedRate(1, recordingJob, TIME_MILLIS);
	int taskId2 = taskManager.scheduleFixedRate(100, recordingJob2, TIME_MICROS);


	// now check the task registration in detail.
	TEST_ASSERT_NOT_EQUAL(taskId1, TASKMGR_INVALIDID);
	TimerTask* task = taskManager.getFirstTask();
    TEST_ASSERT_NOT_NULL(task);
	TEST_ASSERT_FALSE(task->isJobInMillis());
    TEST_ASSERT_TRUE(task->isJobInMicros());
    TEST_ASSERT_FALSE(task->isJobInSeconds());
	
	// now check the task registration in detail.
    TEST_ASSERT_NOT_EQUAL(taskId2, TASKMGR_INVALIDID);
	task = task->getNext();
    TEST_ASSERT_NOT_NULL(task);
    TEST_ASSERT_TRUE(task->isJobInMillis());
    TEST_ASSERT_FALSE(task->isJobInMicros());
    TEST_ASSERT_FALSE(task->isJobInSeconds());

	dumpTasks();

	uint32_t timeStartYield = millis();
	taskManager.yieldForMicros(20000);
	uint32_t timeTaken = millis() - timeStartYield;

	dumpTasks();

	// make sure the yield timings were in range.
	TEST_ASSERT_LESS_OR_EQUAL((uint32_t) 22, timeTaken);
	TEST_ASSERT_GREATER_OR_EQUAL((uint32_t) 19, timeTaken);

	// now make sure that we got in the right ball park of calls.
    TEST_ASSERT_GREATER_OR_EQUAL(15, count);
    TEST_ASSERT_GREATER_OR_EQUAL(150, count2);
}

void testInterruptSupportMarshalling() {
    prepareTesting();

    MockedIoAbstraction mockIo;
	taskManager.setInterruptCallback(intHandler);
	taskManager.addInterrupt(&mockIo, 2, CHANGE);

	// make sure the interrupt is properly registered.
    TEST_ASSERT_TRUE(mockIo.isIntRegisteredAs(2, CHANGE));

	// now pretend the interrupt took place.
	(mockIo.getInterruptFunction())();

	// and wait for task manager to schedule.
	assertThatTaskRunsOnTime(0, 250);
	
	// and the pin should be 2
	TEST_ASSERT_EQUAL(2, pinNo);
}

void testCancellingAJobAfterCreation() {
    prepareTesting();

    TEST_ASSERT_NOT_NULL(taskManager.getFirstTask());

	int taskId = taskManager.scheduleFixedRate(10, recordingJob, TIME_MILLIS);
	
	// now check the task registration in detail.
    TEST_ASSERT_NOT_EQUAL(taskId, TASKMGR_INVALIDID);
	TimerTask* task = taskManager.getFirstTask();
    TEST_ASSERT_NOT_NULL(task);
    TEST_ASSERT_TRUE(task->isJobInMillis());
    TEST_ASSERT_FALSE(task->isJobInMicros());
    TEST_ASSERT_FALSE(task->isJobInSeconds());
	TEST_ASSERT_GREATER_OR_EQUAL(8000UL, task->microsFromNow());

	assertThatTaskRunsOnTime(10000, MILLIS_ALLOWANCE);

	// cancel the task and make sure everything is cleared down
	assertTasksSpacesTaken(1);
	taskManager.cancelTask(taskId);
	assertTasksSpacesTaken(0);

	TEST_ASSERT_NULL(taskManager.getFirstTask());
}

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

//
// This test actually runs the task manager full up with tasks for around 15 seconds, scheduling using
// lots of different intervals from micros through to seconds, using both single shot and repeating
// schedules, this is the most important test to pass in the whole suite.
//

/**
 * This method checks that taskmanager tasks are in proper and stable order. And that only running tasks
 * are in the linked list.
 */
void assertTasksAreInOrder() {
    bool inOrder = true;
    TimerTask* task = taskManager.getFirstTask();
    unsigned long prevTaskMicros = 0;
    while(inOrder && task != NULL) {
        unsigned long currentTaskMicros = task->microsFromNow();
        // the task must be in use
        inOrder = inOrder && task->isInUse();

        // we then compare it in order, obviously millis tick slower than micros
        if(currentTaskMicros < prevTaskMicros && (prevTaskMicros - currentTaskMicros) > 1000) {
            inOrder = false;
            Serial.print("Failed prev "); Serial.print(prevTaskMicros);
            Serial.print(", current ");Serial.println(currentTaskMicros);
        }

        // get the next item and store this micros for next compare.
        prevTaskMicros = currentTaskMicros;
        task = task->getNext();
    }

    // if somehting goes wrong, dump out the whole lot!
    if(!inOrder) dumpTasks();

    // assert that it's in order.
    TEST_ASSERT_TRUE(inOrder);
}

void clearCounts() {
    for(int i=0;i<6;i++) counts[i]=0;
    taskManager.reset();
}


void testTaskManagerHighThroughput() {
	char slotData[15];
    prepareTesting();
    clearCounts();

	Serial.print("Dumping threads"); Serial.println(taskManager.checkAvailableSlots(slotData));

	taskManager.scheduleFixedRate(10, testCall1);
	taskManager.scheduleFixedRate(100, testCall2);
	taskManager.scheduleFixedRate(250, testCall3, TIME_MICROS);
	taskManager.scheduleOnce(1, testCall4, TIME_SECONDS);
	taskManager.scheduleOnce(10, testCall5, TIME_SECONDS);

	Serial.print("Dumping threads"); Serial.println(taskManager.checkAvailableSlots(slotData));

	unsigned long start = millis();
	while(counts[5] < 10 && (millis() - start) < 25000) {
		taskManager.yieldForMicros(10000);
		assertTasksAreInOrder();
	}

	Serial.print("Dumping threads"); Serial.println(taskManager.checkAvailableSlots(slotData));

	TEST_ASSERT_EQUAL(counts[5], 10);               // should be 10 runs as it's manually repeating
	TEST_ASSERT_GREATER_OR_EQUAL(140, counts[1]);   // should be at least 140 runs, scheduled every 100 millis,
    TEST_ASSERT_GREATER_OR_EQUAL(14, counts[3]);    // should be at least 14 runs, as this test lasts about 20 seconds.
    TEST_ASSERT_GREATER_OR_EQUAL(1400, counts[0]);  // should be at least 1400 runs it's scheduled every 10 millis
	TEST_ASSERT_EQUAL(counts[4], 1);                // should have been triggered once
	TEST_ASSERT_NOT_EQUAL(counts[2], 0);            // meaningless to count micros calls. check it happened
}

//
// This test cleans down task manager and then tries to cancel a job within a running task,
// it then waits for the other jobs scheduled after the cancelled to run. See github #38
// Kindly isolated and reported by @martin-klima
//
uint8_t taskId1;
bool taskCancelled;
int storedCount1;
int storedCount2;

void testCancellingTasksWithinAnotherTask() {
	char slotData[15];
    prepareTesting();

    // set up for the run by clearing down state.
	taskManager.reset();
	counts[0] = counts[1] = counts[2] = 0;
	storedCount1 = storedCount2 = 0;
	taskCancelled = false;

	// register three tasks, the first is to be cancelled.
	taskId1 = taskManager.scheduleFixedRate(100, testCall1);
	taskManager.scheduleFixedRate(100, testCall2);
	taskManager.scheduleFixedRate(100, testCall3);

	// schedule the job to cancel the first registered task.
	taskManager.scheduleOnce(500, [] {
		taskManager.cancelTask(taskId1);
		taskCancelled = true;
		storedCount1 = counts[1];
		storedCount2 = counts[2];
	});

	assertTasksAreInOrder();

	// now run the task manager until the job gets cancelled (or it times out)
	int count = 1000;
	while ((--count != 0) && !taskCancelled) {
		taskManager.yieldForMicros(1000L);
	}

	// the cancelled job should have run at least once before cancellation
	// and then must have been cancelled. Tasks should be in order
	TEST_ASSERT_NOT_EQUAL(0, counts[0]);
	TEST_ASSERT_TRUE(taskCancelled);
	
	count = 500;
	while (--count != 0) {
		taskManager.yieldForMicros(1000L);
	}

	// once the task manager has been scheduled again, the call counts should not be the same and the tasks
	// should remain in order
	assertTasksAreInOrder();
	
	// in this case we dump the queue, something is wrong.
	if (counts[1] == storedCount1) {
		dumpTasks();
		Serial.print("Dumping threads"); Serial.println(taskManager.checkAvailableSlots(slotData));
	}

    TEST_ASSERT_NOT_EQUAL(counts[1], storedCount1);
    TEST_ASSERT_NOT_EQUAL(counts[2], storedCount1);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(testRunningUsingExecutorClass);
    RUN_TEST(testSchedulingTaskOnceInMicroseconds);
    RUN_TEST(testSchedulingTaskOnceInMilliseconds);
    RUN_TEST(testSchedulingTaskOnceInSeconds);
    RUN_TEST(testScheduleManyJobsAtOnce);
    RUN_TEST(testScheduleFixedRateTestCase);
    RUN_TEST(testInterruptSupportMarshalling);
    RUN_TEST(testTaskManagerHighThroughput);
    RUN_TEST(testCancellingTasksWithinAnotherTask);
}

void loop() {
    UNITY_END();
}
