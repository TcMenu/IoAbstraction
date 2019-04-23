#ifndef _JOYSTICK_SWITCH_INPUT_H_
#define _JOYSTICK_SWITCH_INPUT_H_

#include "SwitchInput.h"
#include "AnalogDeviceAbstraction.h"

/**
 * @file JoystickSwitchInput.h
 * Provides a rotary encoder emulation based on an analog joystick. Normally used with
 * switches to provide an encoder for your project.
 * 
 * @see SwitchInput
 */

/**
 * This class provides encoder functionality based on an analog joystick. Where up and down
 * increase and reduce the value proportionally to how far down they are pressed.
 * 
 * Normally prefer to create an instance using `setupAnalogJoystickEncoder` in this case there
 * is nothing else to do.
 * 
 * Advanced usages:
 * If you prefer to construct yourself, or want to use more than one encoder, then ensure
 * that you create a task that runs once every 500millis and calls exec.
 */
class JoystickSwitchInput : public RotaryEncoder, public Executable {
private:
    uint8_t analogPin;
    uint8_t counter;
    AnalogDevice* analogDevice;
public:
    /** 
     * Constructor that initialises the class for use, prefer to use the set up method setupAnalogJoystickEncoder
     * in simple cases.
     * @param analogDevice the pointer to the analog AnalogDevice for example: `&analog`
     * @param analogPin the pin on which the joystick analog pin is connected to
     * @param callback the callback to provide updates to when the value changes.
     */
    JoystickSwitchInput(AnalogDevice *analogDevice, uint8_t analogPin, EncoderCallbackFn callback) : RotaryEncoder(callback) {
        this->analogPin = analogPin;
        this->analogDevice = analogDevice;
        analogDevice->initPin(analogPin, DIR_IN);
    }

    int nextInterval(int forceApplied) {
        switch(forceApplied) {
            case 0:
            case 1: return 250;
            case 2: return 200;
            case 3: return 150;
            case 4: return 120;
            case 5: return 100;
            case 6: return 75;
            default:return 50;
        }
    }

    /**
     * Called by taskManager on a frequent basis. Ususally about every 250-500 millis
     */
    void exec() override {
        // this provides 7 times acceleration at full extent.
        int shiftAmt = analogDevice->getBitDepth(DIR_IN, analogPin) - 4;

        int readVal = analogDevice->getCurrentValue(analogPin) >> shiftAmt;
        
        if(readVal == 7 || readVal == 8) {
            // in the probable centre position - do nothing.
            taskManager.scheduleOnce(250, this);
        }
        else if(readVal > 8) {
            // going up!
            int val = readVal - 8;
            increment(-val);
            taskManager.scheduleOnce(nextInterval(val), this);
        }
        else /* less than 7 */ {
            // going down less than 7..
            int val = 7 - readVal;
            increment(val);
            taskManager.scheduleOnce(nextInterval(val), this);
        }
    }
};

/**
 * This is the preferred way to create an instance of a joystick encoder and set it as the
 * default encoder for switches library.
 * @param analogDevice a pointer to an analog device - See example for more detail.
 * @param analogPin the pin onto which the joystick is connected
 * @param callback the callback that will receive changes in value
 */
inline void setupAnalogJoystickEncoder(AnalogDevice* analogDevice, uint8_t analogPin, EncoderCallbackFn callback) {
    auto joystickEncoder = new JoystickSwitchInput(analogDevice, analogPin, callback);
    switches.setEncoder(joystickEncoder);
    taskManager.scheduleOnce(250, joystickEncoder);
}

#endif // _JOYSTICK_SWITCH_INPUT_H_
