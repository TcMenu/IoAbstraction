/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _TIMER_MANAGER_H_
#define _TIMER_MANAGER_H_

typedef void (*TimerFn)();
typedef void (*InterruptFn)(uint8_t);

#define TASKMGR_INVALIDID 0xff

#define TASK_IN_USE     0x8000
#define TASK_REPEATING  0x4000
#define TASK_MILLIS     0x2000
#define TASK_SECONDS    0x1000
#define TASK_MICROS     0x0000
#define TASK_RUNNING    0x0800
#define TIMER_MASK      0x07ff

#define DEFAULT_TASK_SIZE 6

enum TimerUnit : byte {
	TIME_MICROS = 0, TIME_SECONDS = 1, TIME_MILLIS=2
};

class TimerTask {
private:
	uint16_t executionInfo;
	uint32_t scheduledAt;
	TimerFn callback;
	TimerTask* next;
public:
	TimerTask();
	bool isReady();
	unsigned long microsFromNow();
	void initialise(uint16_t executionInfo, TimerFn execCallback);
	inline void execute();
	bool isInUse() { return (executionInfo & TASK_IN_USE) != 0; }
	bool isRepeating() { return (executionInfo & TASK_REPEATING) != 0; }
	void clear();
	void markRunning() { executionInfo |= TASK_RUNNING; }
	void clearRunning() { executionInfo &= ~TASK_RUNNING; }
	bool isRunning() { return (executionInfo & TASK_RUNNING) != 0; }
	TimerTask* getNext() { return next; }
	void setNext(TimerTask* next) { this->next = next; }
};

class TaskManager {
private:
	TimerTask *tasks;
	TimerTask *first;
	uint8_t numberOfSlots;
	InterruptFn interruptCallback;
	volatile uint8_t lastInterruptTrigger;
	volatile bool interrupted;
public:
	TaskManager(uint8_t taskSlots = DEFAULT_TASK_SIZE);

	uint8_t scheduleOnce(int millis, TimerFn timerFunction, TimerUnit timeUnit = TIME_MILLIS);
	uint8_t scheduleFixedRate(int millis, TimerFn timerFunction, TimerUnit timeUnit = TIME_MILLIS);
	void addInterrupt(uint8_t pin, uint8_t mode);
	void setInterruptCallback(InterruptFn handler);
	void cancelTask(uint8_t task);

	void yieldForMicros(uint16_t micros);

	char* checkAvailableSlots(char* slotData);

	// this should be pretty much the only code in loop()
	void runLoop();

	static void markInterrupted(uint8_t interruptNo);
private:
	int findFreeTask();
	void removeFromQueue(TimerTask* task);
	void putItemIntoQueue(TimerTask* tm);
};

extern TaskManager taskManager;

#endif
