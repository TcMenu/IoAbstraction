#include <inttypes.h>
#include <Arduino.h>
#include "TaskManager.h"


TimerTask::TimerTask() {
	clear();
}

void TimerTask::initialise(uint16_t executionInfo, TimerFn execCallback) {
	this->executionInfo = executionInfo;
	this->callback = execCallback;
	
	this->scheduledAt = (executionInfo & TASK_MICROS) ? micros() : millis();
}

bool TimerTask::isReady(unsigned long now) {
	if (!isInUse()) return false;

	// TODO handle clock wrapping??

	if ((executionInfo & TASK_MICROS) != 0) {
		return (scheduledAt + (executionInfo & TIMER_MASK)) <= micros();
	}
	else {
		uint32_t startTm = (executionInfo & TIMER_MASK);
		if ((executionInfo & TASK_SECONDS) != 0) {
			startTm *= 1000;
		}
		return (scheduledAt + startTm) <= millis();
	}
}

void TimerTask::execute() {
	if (callback == NULL) return; // failsafe, always exit with null callback.

	callback();

	// handle the two cases, repeating tasks - reschedule, or free up.
	if (isRepeating()) {
		this->scheduledAt = (executionInfo & TASK_MICROS) ? micros() : millis();
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


TaskManager* TaskManager::__taskMgrInstance;

void TaskManager::markInterrupted(uint8_t interruptNo) {
	__taskMgrInstance->lastInterruptTrigger = interruptNo;
	__taskMgrInstance->interrupted = true;
}

TaskManager::TaskManager(uint8_t taskSlots = DEFAULT_TASK_SIZE) {
	this->numberOfSlots = taskSlots;
	this->tasks = new TimerTask[taskSlots];
	interrupted = false;
}

int TaskManager::findFreeTask() {
	for (uint8_t i = 0; i < numberOfSlots; ++i) {
		if (!tasks[i].isInUse()) {
			return i;
		}
	}
	return -1;
}

inline int toTimerValue(int v, TimerUnit unit) {
	if (unit == TIME_MILLIS && v > 4096) {
		unit = TIME_SECONDS;
		v = v / 1000;
	}
	v = min(v, TIMER_MASK);
	return v | (((uint16_t)unit) << 12);
}

uint8_t TaskManager::scheduleOnce(int millis, TimerFn timerFunction, TimerUnit timeUnit = TIME_MILLIS) {
	uint8_t taskId = findFreeTask();
	if (taskId >= 0) {
		tasks[taskId].initialise(toTimerValue(millis, timeUnit) | TASK_IN_USE, timerFunction);
	}
	return taskId;
}

uint8_t TaskManager::scheduleFixedRate(int millis, TimerFn timerFunction, TimerUnit timeUnit = TIME_MILLIS) {
	uint8_t taskId = findFreeTask();
	if (taskId >= 0) {
		tasks[taskId].initialise(toTimerValue(millis, timeUnit) | TASK_IN_USE | TASK_REPEATING, timerFunction);
	}
	return taskId;
}

void TaskManager::cancelTask(uint8_t task) {
	if (task >= 0 && task < numberOfSlots) {
		tasks[task].clear();
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

