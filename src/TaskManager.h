/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _TIMER_MANAGER_H_
#define _TIMER_MANAGER_H_

#include "BasicIoAbstraction.h"

/** 
 * @file TaskManager.h
 * 
 * Task manager is a simple co-routine style implementation for Arduino which supports basic task scheduling.
 */

#ifdef __AVR__
#define DEFAULT_TASK_SIZE 6
#else
#define DEFAULT_TASK_SIZE 10
#endif

//
// Only define this if you have libraries you are using that have long delays that you cannot control. Be aware this
// comes with a lot of side effects, and you should check carefully that it works for you before using.
// For sure, the library you are using will need protecting from more than one task accessing it. (EG: run from only one task)
//
// default off, uncomment line below to enable - test carefully!!
//
// #define _TASKMGR_OVERRIDE_DELAY_ 1
//
#if defined(_TASKMGR_OVERRIDE_DELAY_)

/**
 * TaskManager can override the delay behaviour controlled by _TASKMGR_OVERRIDE_DELAY_ in TaskManager.h (default off)
 * Instead of holding everything up this option keeps the task queue running. Only enable after testing for your cases.
 * 
 * This tries to run tasks during the time that delay is called, returning after at least x micros.
 * @param x the number of microseconds to wait
 */
#define delayMicroseconds(x) (taskManager.yieldForMicros(x))
#endif

// END User adjustable values

/**
 * Definition of a function to be called back when a scheduled event is required. Takes no parameters, returns nothing.
 */
typedef void (*TimerFn)();

/**
 * Definition of a function to be called back when an interrupt is detected, marshalled by task manager into a task.
 * The pin that caused the interrupt is passed in the parameter on a best efforts basis.
 * @param pin the pin on which the interrupt occurred (best efforts)
 */ 
typedef void (*InterruptFn)(uint8_t pin);

#define TASKMGR_INVALIDID 0xff

#define TASK_IN_USE     0x8000
#define TASK_REPEATING  0x4000
#define TASK_MILLIS     0x2000
#define TASK_SECONDS    0x1000
#define TASK_MICROS     0x0000
#define TASK_RUNNING    0x0800
#define TIMER_MASK      0x07ff

/**
 * The time units that can be used with the schedule calls.
 */
enum TimerUnit : byte {
	TIME_MICROS = 0, TIME_SECONDS = 1, TIME_MILLIS=2
};

/**
 * Internal class only that represents a single task slot.
 */
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

/**
 * TaskManager is a lightweight co-routine implementation for Arduino, it works by scheduling tasks to be
 * done either immediately or at a future point in time. It is quite efficient at scheduling tasks as 
 * internally tasks are arranged in time order in a linked list.
 * 
 * There is a globally defined variable called `taskManager`, do not create more instances of this class.
 */
class TaskManager {
private:
	TimerTask tasks[6];
	TimerTask *first;
	uint8_t numberOfSlots;
	InterruptFn interruptCallback;
	volatile uint8_t lastInterruptTrigger;
	volatile bool interrupted;
public:
	/**
	 * Do not construct this class manually, there is a global instance called `taskManager`
	 */
	TaskManager();

	/**
	 * Schedules a task for one shot execution in the timeframe provided.
	 * @param millis the time frame in which to schedule the task
	 * @param timerFunction the function to run at that time
	 * @param timeUnit defaults to TIME_MILLIS but can be any of the possible values.
	 */
	uint8_t scheduleOnce(int millis, TimerFn timerFunction, TimerUnit timeUnit = TIME_MILLIS);

	/**
	 * Schedules a task for repeated execution at the frequency provided.
	 * @param millis the frequency at which to execute
	 * @param timerFunction the function to run at that time
	 * @param timeUnit defaults to TIME_MILLIS but can be any of the possible values.
	 */
	uint8_t scheduleFixedRate(int millis, TimerFn timerFunction, TimerUnit timeUnit = TIME_MILLIS);

	/**
	 * Adds an interrupt that will be handled by task manager, such that it's marshalled into a task.
	 * This registers an interrupt with any IoAbstractionRef.
	 * @param ref the IoAbstractionRef that we want to register the interrupt for
	 * @param pin the pin upon which to register (on the IoDevice above)
	 * @param mode the mode in which to register, eg. CHANGE, RISING, FALLING
	 */
	void addInterrupt(IoAbstractionRef ref, uint8_t pin, uint8_t mode);
	
	/**
	 * Sets the interrupt callback to be used when an interrupt is signalled. Note that you will be
	 * called back by task manager, and you are safe to use any variables as normal. Task manager
	 * marshalls the interrupt for you.
	 * @param handler the interrupt handler
	 */
	void setInterruptCallback(InterruptFn handler);
	
	/**
	 * Stop a task from executing or cancel it from executing again if it is a repeating task
	 * @param task the task ID returned from the schedule call
	 */ 
	void cancelTask(uint8_t task);

	/**
	 * Use instead of delays or wait loops inside code that needs to perform timing functions. It will
	 * not call back until at least `micros` time has passed.
	 * @param micros the number of microseconds to wait.
	 */  
	void yieldForMicros(uint16_t micros);

	/**
	 * This method fills slotData with the current running conditions of each available task slot.
	 * Useful to ensure that you're not overloading taskManager, or if the number of tasks need
	 * be increased.
	 * @param slotData the char array to fill in with task information. Must be as long as number of tasks.
	 */
	char* checkAvailableSlots(char* slotData);

	/**
	 * This should be called in the loop() method of your sketch, ensure that your loop method does
	 * not do anything that will unduly delay calling this method.
	 */
	void runLoop();

	/**
	 * Used internally by the interrupt handlers to tell task manager an interrupt is waiting. Not for external use.
	 */
	static void markInterrupted(uint8_t interruptNo);
private:
	int findFreeTask();
	void removeFromQueue(TimerTask* task);
	void putItemIntoQueue(TimerTask* tm);
};

/** the global task manager, always use this instance. */
extern TaskManager taskManager;

#endif
