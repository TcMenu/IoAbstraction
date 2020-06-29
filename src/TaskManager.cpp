/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifdef __MBED__
#include <mbed.h>
#include "rtos.h"
#else
#include <Arduino.h>
#endif

#include "IoAbstraction.h"
#include "TaskManager.h"

#undef ISR_ATTR
#if defined(ESP8266)
# define ISR_ATTR ICACHE_RAM_ATTR
#elif defined(ESP32)
# define ISR_ATTR IRAM_ATTR
#else
# define ISR_ATTR
#endif

TaskManager taskManager;

TimerTask::TimerTask() {
	next = NULL;
	clear();
}

void TimerTask::initialise(uint16_t executionInfo, TimerFn execCallback) {
	this->executionInfo = executionInfo;
	this->callback = execCallback;
	this->executableJob = false;
	
	this->scheduledAt = (isJobMicros(executionInfo)) ? micros() : millis();
	this->next = NULL;
}

void TimerTask::initialise(uint16_t executionInfo, Executable* execCallback) {
	this->executionInfo = executionInfo;
	this->taskRef = execCallback;
	this->executableJob = true;
	
	this->scheduledAt = (isJobMicros(executionInfo)) ? micros() : millis();
	this->next = NULL;
}

bool TimerTask::isReady() {
	if (!isInUse() || isRunning()) return false;

	if ((isJobMicros(executionInfo)) != 0) {
		uint16_t delay = timeFromExecInfo(executionInfo);
		return (micros() - scheduledAt) >= delay;
	}
	else if(isJobSeconds(executionInfo)) {
		uint32_t delay = timeFromExecInfo(executionInfo) * 1000L;
		return (millis() - scheduledAt) >= delay;
	}
	else {
		uint16_t delay = timeFromExecInfo(executionInfo);
		return (millis() - scheduledAt) >= delay;
	}
}

unsigned long TimerTask::microsFromNow() {
	uint32_t microsFromNow;
	if (isJobMicros(executionInfo)) {
		uint16_t delay = timeFromExecInfo(executionInfo);
		int32_t alreadyTaken = (micros() - scheduledAt);
		microsFromNow =  (delay < alreadyTaken) ? 0 : (delay - alreadyTaken);
	}
	else {
		uint32_t delay = timeFromExecInfo(executionInfo);
		if (isJobSeconds(executionInfo)) {
			delay *= ((uint32_t)1000);
		}
		uint32_t alreadyTaken = (millis() - scheduledAt);
		microsFromNow = (delay < alreadyTaken) ? 0 : ((delay - alreadyTaken) * 1000UL);
	}
	return microsFromNow;
}


inline void TimerTask::execute() {
	if (callback == NULL) return; // failsafe, always exit with null callback.

	// handle repeating tasks - reschedule.
	if (isRepeating()) {
		markRunning();
		if(executableJob) {
			taskRef->exec();
		}
		else {
			callback();
		}
		this->scheduledAt = isJobMicros(executionInfo) ? micros() : millis();
		clearRunning();
	}
	else {
		void* savedCallback = (void*)callback;
		clear();
		if(executableJob) {
			((Executable*)savedCallback)->exec();
		}
		else {
			((TimerFn)savedCallback)();
		}
	}
}

void TimerTask::clear() {
	executionInfo = 0;
	callback = NULL;
	next = NULL;
}

ISR_ATTR void TaskManager::markInterrupted(pinid_t interruptNo) {
	taskManager.lastInterruptTrigger = interruptNo;
	taskManager.interrupted = true;
}

TaskManager::TaskManager() {
	this->numberOfSlots = DEFAULT_TASK_SIZE;
	interrupted = false;
	first = NULL;
	interruptCallback = NULL;
	tasks = new TimerTask[DEFAULT_TASK_SIZE];
}

int TaskManager::findFreeTask() {
	for (uint8_t i = 0; i < numberOfSlots; ++i) {
		if (!tasks[i].isInUse()) {
			return i;
		}
	}

	int newSlots = numberOfSlots + DEFAULT_TASK_SIZE;
	auto newTasks = new TimerTask[newSlots];
	if(newTasks != NULL) {
	    int firstNewSlot = numberOfSlots;
	    memcpy(newTasks, tasks, sizeof(TimerTask) * numberOfSlots);
	    tasks = newTasks;
	    numberOfSlots = newSlots;
        serdebugF3("Successful realloc(slots,ret) ", numberOfSlots, firstNewSlot);

        return firstNewSlot;
	}

    serdebugF("No task slot found");
	return TASKMGR_INVALIDID;
}

inline uint16_t toTimerValue(uint16_t v, TimerUnit unit) {
	if (unit == TIME_MILLIS && v > TIMER_MASK) {
		unit = TIME_SECONDS;
		v = v / 1000U;
	}
	return (v & TIMER_MASK) | (((uint16_t)unit) << 12);
}

uint8_t TaskManager::scheduleOnce(uint16_t when, TimerFn timerFunction, TimerUnit timeUnit) {
	uint8_t taskId = findFreeTask();
	if (taskId != TASKMGR_INVALIDID) {
		tasks[taskId].initialise(toTimerValue(when, timeUnit) | TASK_IN_USE, timerFunction);
		putItemIntoQueue(&tasks[taskId]);
	}
	return taskId;
}

uint8_t TaskManager::scheduleFixedRate(uint16_t when, TimerFn timerFunction, TimerUnit timeUnit) {
	uint8_t taskId = findFreeTask();
	if (taskId != TASKMGR_INVALIDID) {
		tasks[taskId].initialise(toTimerValue(when, timeUnit) | TASK_IN_USE | TASK_REPEATING, timerFunction);
		putItemIntoQueue(&tasks[taskId]);
	}
	return taskId;
}

uint8_t TaskManager::scheduleOnce(uint16_t when, Executable* execRef, TimerUnit timeUnit) {
	uint8_t taskId = findFreeTask();
	if (taskId != TASKMGR_INVALIDID) {
		tasks[taskId].initialise(toTimerValue(when, timeUnit) | TASK_IN_USE, execRef);
		putItemIntoQueue(&tasks[taskId]);
	}
	return taskId;
}

uint8_t TaskManager::scheduleFixedRate(uint16_t when, Executable* execRef, TimerUnit timeUnit) {
	uint8_t taskId = findFreeTask();
	if (taskId != TASKMGR_INVALIDID) {
		tasks[taskId].initialise(toTimerValue(when, timeUnit) | TASK_IN_USE | TASK_REPEATING, execRef);
		putItemIntoQueue(&tasks[taskId]);
	}
	return taskId;
}

void TaskManager::cancelTask(uint8_t task) {
	if (task < numberOfSlots) {
		removeFromQueue(&tasks[task]);
		tasks[task].clear();
	}
}

void TaskManager::yieldForMicros(uint16_t microsToWait) {
	yield();

	unsigned long microsStart = micros();
	do {
        runLoop();
	} while((micros() - microsStart) < microsToWait);
}

void TaskManager::runLoop() {
	// when there's an interrupt, we marshall it into a timer interrupt.
	if (interrupted) {
		interrupted = false;
		interruptCallback(lastInterruptTrigger);
	}

	// go through the timer (scheduled) tasks in priority order. they are stored
	// in a linked list ordered by first to be scheduled. So we only go through
	// these until the first one that isn't ready.
	TimerTask* tm = first;
	while(tm != NULL) {
#ifdef ESP8266
	    // here we are making extra sure we are good citizens on ESP boards
	    yield();
#endif
		if (tm->isReady()) {
			removeFromQueue(tm);
			tm->execute();
			if (tm->isRepeating()) {
				putItemIntoQueue(tm);
			}
		}
		else {
			break;
		}

		tm = tm->getNext();
	}
}

void TaskManager::putItemIntoQueue(TimerTask* tm) {

	// shortcut, no first yet, so we are at the top!
	if (first == NULL) {
		first = tm;
		tm->setNext(NULL);
		return;
	}

	// if we are the new first..
	if (first->microsFromNow() > tm->microsFromNow()) {
		tm->setNext(first);
		first = tm;
		return;
	}

	// otherwise we have to find the place in the queue for this item by time
	TimerTask* current = first->getNext();
	TimerTask* previous = first;

	while (current != NULL) {
		if (current->microsFromNow() > tm->microsFromNow()) {
			previous->setNext(tm);
			tm->setNext(current);
			return;
		}
		previous = current;
		current = current->getNext();
	}

	// we are at the end of the queue
	previous->setNext(tm);
	tm->setNext(NULL);
}

void TaskManager::removeFromQueue(TimerTask* tm) {
	
	// shortcut, if we are first, just remove us by getting the next and setting first.
	if (first == tm) {
		first = tm->getNext();
		tm->setNext(NULL);
		return;
	}

	// otherwise, we have a single linked list, so we need to keep previous and current and
	// then iterate through each item
	TimerTask* current = first->getNext();
	TimerTask* previous = first;

	while (current != NULL) {

		// we've found the item, unlink it from the queue and nullify its next.
		if (current == tm) {
			previous->setNext(current->getNext());
			current->setNext(NULL);
			break;
		}

		previous = current;
		current = current->getNext();
	}
}

typedef void ArdunioIntFn(void);

ISR_ATTR void interruptHandler1() {
	taskManager.markInterrupted(1);
}
ISR_ATTR void interruptHandler2() {
	taskManager.markInterrupted(2);
}
ISR_ATTR void interruptHandler3() {
	taskManager.markInterrupted(3);
}
ISR_ATTR void interruptHandler4() {
	taskManager.markInterrupted(4);
}
ISR_ATTR void interruptHandler5() {
	taskManager.markInterrupted(5);
}
ISR_ATTR void interruptHandler6() {
	taskManager.markInterrupted(6);
}
ISR_ATTR void interruptHandler7() {
	taskManager.markInterrupted(7);
}
ISR_ATTR void interruptHandler8() {
	taskManager.markInterrupted(8);
}
ISR_ATTR void interruptHandler9() {
	taskManager.markInterrupted(9);
}
ISR_ATTR void interruptHandler10() {
	taskManager.markInterrupted(10);
}
ISR_ATTR void interruptHandler11() {
	taskManager.markInterrupted(11);
}
ISR_ATTR void interruptHandler12() {
	taskManager.markInterrupted(12);
}
ISR_ATTR void interruptHandler13() {
	taskManager.markInterrupted(13);
}
ISR_ATTR void interruptHandler14() {
	taskManager.markInterrupted(14);
}
ISR_ATTR void interruptHandler15() {
	taskManager.markInterrupted(15);
}
ISR_ATTR void interruptHandler18() {
	taskManager.markInterrupted(18);
}
ISR_ATTR void interruptHandlerOther() {
	taskManager.markInterrupted(0xff);
}

void TaskManager::addInterrupt(IoAbstractionRef ioDevice, pinid_t pin, uint8_t mode) {
	if (interruptCallback == NULL) return;

	switch (pin) {
	case 1: ioDevice->attachInterrupt(pin, interruptHandler1, mode); break;
	case 2: ioDevice->attachInterrupt(pin, interruptHandler2, mode); break;
	case 3: ioDevice->attachInterrupt(pin, interruptHandler3, mode); break;
	case 4: ioDevice->attachInterrupt(pin, interruptHandler4, mode); break;
	case 5: ioDevice->attachInterrupt(pin, interruptHandler5, mode); break;
	case 6: ioDevice->attachInterrupt(pin, interruptHandler6, mode); break;
	case 7: ioDevice->attachInterrupt(pin, interruptHandler7, mode); break;
	case 8: ioDevice->attachInterrupt(pin, interruptHandler8, mode); break;
	case 9: ioDevice->attachInterrupt(pin, interruptHandler9, mode); break;
	case 10: ioDevice->attachInterrupt(pin, interruptHandler10, mode); break;
	case 11: ioDevice->attachInterrupt(pin, interruptHandler11, mode); break;
	case 12: ioDevice->attachInterrupt(pin, interruptHandler12, mode); break;
	case 13: ioDevice->attachInterrupt(pin, interruptHandler13, mode); break;
	case 14: ioDevice->attachInterrupt(pin, interruptHandler14, mode); break;
	case 15: ioDevice->attachInterrupt(pin, interruptHandler15, mode); break;
	case 18: ioDevice->attachInterrupt(pin, interruptHandler18, mode); break;
	default: ioDevice->attachInterrupt(pin, interruptHandlerOther, mode); break;
	}
}

void TaskManager::setInterruptCallback(InterruptFn handler) {
	interruptCallback = handler;
}

char* TaskManager::checkAvailableSlots(char* data) {
	uint8_t i;
	for (i = 0; i < numberOfSlots; ++i) {
		data[i] = tasks[i].isRepeating() ? 'R' : (tasks[i].isInUse() ? 'U' : 'F');
		if (tasks[i].isRunning()) data[i] = tolower(data[i]);
	}
	data[i] = 0;
	return data;
}

#ifdef __MBED__

volatile bool timingStarted = false;
Timer ioaTimer;

void yield() {
    ThisThread::yield();
}
unsigned long millis() {
    if(!timingStarted) {
        timingStarted = true;
        ioaTimer.start();
    }
    return ioaTimer.read_ms();
}
unsigned long micros() {
    if(!timingStarted) {
        timingStarted = true;
        ioaTimer.start();
    }
    return (unsigned long) ioaTimer.read_high_resolution_us();
}
#endif