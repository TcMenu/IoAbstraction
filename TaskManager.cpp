#include <inttypes.h>
#include <Arduino.h>
#include "TaskManager.h"


TimerTask::TimerTask() {
	clear();
}

void TimerTask::initialise(uint16_t executionInfo, TimerFn execCallback) {
	this->executionInfo = executionInfo;
	this->callback = execCallback;
	this->nextMillis = millis() + getTimerValue();
}

bool TimerTask::isReady(unsigned long now) {
	// TODO handle clock wrapping..
	return isInUse() && (nextMillis < now);
}

void TimerTask::execute() {
	if (callback == NULL) return; // failsafe, always exit with null callback.

	callback();

	// handle the two cases, repeating tasks - reschedule, or free up.
	if (isRepeating()) {
		this->nextMillis = millis() + getTimerValue();
	}
	else {
		clear();
	}
}

int TimerTask::getTimerValue() {
	int tv = executionInfo & TIMER_MASK;
	if (tv <= 1000) {
		return tv;
	}
	else {
		return tv = (tv - 1000) << 3;
	}
}

void TimerTask::clear() {
	executionInfo = 0;
	callback = NULL;
}


static TaskManager* TaskManager::__taskMgrInstance;

static void TaskManager::markInterrupted(uint8_t interruptNo) {
	__taskMgrInstance->interrupted = true;
	__taskMgrInstance->lastInterruptTrigger = interruptNo;
}

TaskManager::TaskManager(uint8_t taskSlots = DEFAULT_TASK_SIZE) {
	this->numberOfSlots = taskSlots;
	this->tasks = new TimerTask[taskSlots];
	interrupted = false;
}

int TaskManager::findFreeTask() {
	for (uint8_t i = 0; i < numberOfSlots; ++i) {
		if (!tasks[i].isInUse()) {
			Serial.print(i);
			Serial.println("found slot");
			return i;
		}
	}
	return -1;
}

int toTimerValue(int v) {
	if (v <= 1000) {
		return v;
	}
	else {
		return 1000 + (v >> 3);
	}
}

void TaskManager::scheduleOnce(int millis, TimerFn timerFunction) {
	uint8_t taskId = findFreeTask();
	if (taskId >= 0) {
		tasks[taskId].initialise((toTimerValue(millis) & TIMER_MASK) | TASK_IN_USE, timerFunction);
	}
}

void TaskManager::scheduleFixedRate(int millis, TimerFn timerFunction) {
	uint8_t taskId = findFreeTask();
	if (taskId >= 0) {
		tasks[taskId].initialise((toTimerValue(millis) & TIMER_MASK) | TASK_IN_USE | TASK_REPEATING, timerFunction);
	}
}

char* TaskManager::checkAvailableSlots(char* data) {
	uint8_t i;
	for (i = 0; i < numberOfSlots; ++i) {
		data[i] = tasks[i].isRepeating() ? 'R' : (tasks[i].isInUse() ? 'U' : 'F');
	}
	data[i] = 0;
	return data;
}

void TaskManager::runLoop() {
	unsigned long milliStored = millis();
	for (uint8_t i = 0; i < numberOfSlots; ++i) {
		if (tasks[i].isReady(milliStored)) {
			tasks[i].execute();
		}

		if (interrupted) {
			interrupted = false;
			interruptCallback(lastInterruptTrigger);
		}
	}

	// TODO, if not interrupted, and no task for a while sleep and set timer - go into low power.
}

typedef void ArdunioIntFn(void);

void interruptHandler1() {
	TaskManager::__taskMgrInstance->markInterrupted(1);
}
void interruptHandler2() {
	TaskManager::__taskMgrInstance->markInterrupted(2);
}
void interruptHandler3() {
	TaskManager::__taskMgrInstance->markInterrupted(3);
}
void interruptHandler5() {
	TaskManager::__taskMgrInstance->markInterrupted(5);
}
void interruptHandler18() {
	TaskManager::__taskMgrInstance->markInterrupted(18);
}
void interruptHandlerOther() {
	TaskManager::__taskMgrInstance->markInterrupted(0xff);
}

void TaskManager::addInterrupt(uint8_t pin, uint8_t mode) {
	if (interruptCallback == NULL) return;
	TaskManager::__taskMgrInstance = this;
	int interruptNo = digitalPinToInterrupt(pin);

	switch (pin) {
	case 1:  attachInterrupt(interruptNo, interruptHandler1, mode); break;
	case 2: attachInterrupt(interruptNo, interruptHandler2, mode); break;
	case 3: attachInterrupt(interruptNo, interruptHandler3, mode); break;
	case 5: attachInterrupt(interruptNo, interruptHandler5, mode); break;
	case 18: attachInterrupt(interruptNo, interruptHandler18, mode); break;
	default: attachInterrupt(interruptNo, interruptHandlerOther, mode); break;
	}
}

void TaskManager::setInterruptCallback(InterruptFn handler) {
	interruptCallback = handler;
}

