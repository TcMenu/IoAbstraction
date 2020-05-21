/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */
#ifndef _MOCKED_TASK_MANAGER_H_
#define _MOCKED_TASK_MANAGER_H_

/**
 * @ file MockTaskManager.h
 * 
 * This file contains a version of task manager that is useful for dev & testing.
 * None of the implementations in this file are designed for production use.
 */

#include <TaskManager.h>

/** 
 * This adds a bit of extra stuff to task manager for testing. Never call the runLoop
 * method, as it will try and schedule directly. Instead manually use the helper methods
 * to run the scheduled tasks manaually during test running.
 */
class SimulatedTaskManager : public TaskManager {
private:
    uint16_t yieldTimes[10];
    uint8_t numOfYields;
public:
    SimulatedTaskManager() {
        reset();
    }

    void reset() {
        numOfYields = 0;
        for(int i =0; i<numberOfSlots; i++) {
            tasks[i].clear();
        }
        TaskManager::reset();
    }

    void yieldForMicros(uint16_t micros) override {
        if(numOfYields >= 10) return;
        yieldTimes[numOfYields++] = micros;
    }

    int getNumberOfYieldCalls() {return numOfYields;}
    uint16_t getYieldTime(int i) {return yieldTimes[i];}
    TimerTask* getTask(int i) {return &tasks[i];}
    int getMaxTaskNo() {return numberOfSlots;}
    InterruptFn getInterruptFunction() {return interruptCallback;}
};

#endif // _MOCKED_TASK_MANAGER_H_
