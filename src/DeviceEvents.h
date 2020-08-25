//
// Created by David Cherry on 22/08/2020.
//

#ifndef IOABSTRACTION_DEVICEEVENTS_H
#define IOABSTRACTION_DEVICEEVENTS_H

#include "TaskManagerIO.h"
#include "PlatformDetermination.h"
#include "AnalogDeviceAbstraction.h"



/**
 * Here we create our event class, all events must extend from BaseEvent, and that class provides much of the boiler
 * plate needed for event handling. Here we implement a very simple latching analog level threshold event.
 */
class AnalogInEvent : public BaseEvent {
public:
    /**
     * Describes the way in which the Analog event should trigger.
     */
    enum AnalogEventMode {
        /** Trigger the event when it exceeds the threshold */
        ANALOGIN_EXCEEDS,
        /** Trigger the event when it goes below the threshold */
        ANALOGIN_BELOW,
        /** Trigger the event when it changes by more than threshold */
        ANALOGIN_CHANGE
    };
private:
    AnalogDevice *analogDevice;
    AnalogEventMode mode;
    uint32_t pollInterval;
    bool latched;
    pinid_t analogPin;
protected:
    float analogThreshold;
    float lastReading;
public:


    /**
     * Constructs the event class providing the analog pin to read from and the threshold for triggering.
     * @param inputPin the pin to read from
     * @param threshold the value at which to trigger the event.
     */
    AnalogInEvent(AnalogDevice *device, pinid_t inputPin, float threshold, AnalogEventMode mode_,
                  uint32_t pollInterval_) : BaseEvent() {
        analogThreshold = threshold;
        analogPin = inputPin;
        lastReading = 0;
        pollInterval = pollInterval_;
        analogDevice = device;
        latched = false;
        mode = mode_;
    }

    /**
     * Change to another polling interval
     * @param micros the new polling interval in microseconds
     */
    void setPollInterval(uint32_t micros) {
        pollInterval = micros;
    }

    /**
     * Implementation of the method that checks the analog reading against the condition for this instance. If the
     * condition is met, then it triggers the event, which stays latched until the condition  is no longer met, and
     * then it is unlatched.
     * @return the configured poll interval.
     */
    uint32_t timeOfNextCheck() override {
        lastReading = analogDevice->getCurrentFloat(analogPin);
        auto analogTrigger = isConditionTrue();
        if (analogTrigger && !latched) {
            setTriggered(true);
            latched = true;
        }
        else if(!analogTrigger && latched) {
            latched = false;
        }
        return pollInterval;
    }

    /**
     * Checks if the condition for the event is met, IE if the analog in value is within the range for the interrupt.
     * @return true if the condition is met, otherwise false.
     */
    bool isConditionTrue() {
        if (mode == ANALOGIN_BELOW) {
            return lastReading < analogThreshold;
        }
        else if(mode == ANALOGIN_EXCEEDS) {
            return lastReading > analogThreshold;
        }
        else {
            auto change = abs(analogThreshold - lastReading);
            return change > analogThreshold;
        }
    }

    /**
     * Non-polling case, change interrupt attached to analog pin.
     * We've been notified that a reading available from interrupt, tell taskmanager to run event evaluation now.
     * This will cause all events to run a check followed by the trigger if needed. Completely safe to call
     * from interrupts and other threads
     */
    void readingAvailable() {
        taskManager.triggerEvents();
    }
};

#endif //IOABSTRACTION_DEVICEEVENTS_H
