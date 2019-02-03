#ifndef _DF_ROBOT_INPUT_ABSTRACTION_H
#define _DF_ROBOT_INPUT_ABSTRACTION_H

#include "BasicIoAbstraction.h"

/**
 * @file DfRobotInputAbstraction.h
 * 
 * This file contains an implementation of BasicIoAbstraction that works with the DfRobot shield
 * in order to convert the analog keys into regular digital IO. This allows DfRobot to be used
 * with switches for button management.
 * 
 * See: https://www.dfrobot.com/wiki/index.php/Arduino_LCD_KeyPad_Shield_(SKU:_DFR0009)
 */

#define DF_KEY_RIGHT 0
#define DF_KEY_LEFT 1
#define DF_KEY_UP 2
#define DF_KEY_DOWN 3
#define DF_KEY_SELECT 4

struct DfRobotAnalogRanges {
    uint16_t right;
    uint16_t up;
    uint16_t down;
    uint16_t left;
    uint16_t select;
};

#define ALLOWABLE_RANGE 8
#define pgmAsInt(x) ((int)pgm_read_word_near(x))
/**
 * DfRobotInputAbstraction provides the means to use many buttons connected to a single
 * Analog input. It is mainly designed to work with the df robot shield.
 * It simulates a digital port of 8 bits where the following mappings are made
 * 
 * * pin0 = right (DF_KEY_RIGHT)
 * * pin1 = up (DF_KEY_UP)
 * * pin2 = down (DF_KEY_DOWN)
 * * pin3 = left (DF_KEY_LEFT)
 * * pin4 = select (DF_KEY_SELECT)
 */
class DfRobotInputAbstraction : public BasicIoAbstraction {
private:
    uint8_t analogPin;
    uint8_t readCache;
    int lastReading;
    const DfRobotAnalogRanges* analogRanges;

public:
    DfRobotInputAbstraction(const DfRobotAnalogRanges* ranges, uint8_t pin) {
        analogRanges = ranges;
        analogPin = pin;

        pinMode(analogPin, INPUT);
        lastReading = analogRead(analogPin);
        readCache = mapAnalogToPin(lastReading);
    }

    uint8_t readValue(uint8_t pin) override {
    	return bitRead(readCache, pin);
    }

    uint8_t readPort(uint8_t port) override {
        return readCache;
    }

	bool runLoop() override { 
        int newReading = analogRead(analogPin);
        if(abs(newReading - lastReading) > ALLOWABLE_RANGE) {
            readCache = mapAnalogToPin(newReading);
        }
        lastReading = newReading;
    }

    uint8_t mapAnalogToPin(int reading) {
        uint8_t ret = 0xff;
        if(reading < pgmAsInt(&analogRanges->right)) ret =  DF_KEY_RIGHT;
        else if(reading < pgmAsInt(&analogRanges->up)) ret =  DF_KEY_UP;
        else if(reading < pgmAsInt(&analogRanges->down)) ret = DF_KEY_DOWN;
        else if(reading < pgmAsInt(&analogRanges->left)) ret = DF_KEY_LEFT;
        else if(reading < pgmAsInt(&analogRanges->select)) ret = DF_KEY_SELECT;

        if(ret == 0xff) 
            return 0;
        else
            return 1 << ret;
    }

    // we ignore all non-input methods, as this is input only

    void pinDirection(uint8_t pin, uint8_t mode) override {
        /** ignored as only input is supported */
    }

   	void writeValue(uint8_t pin, uint8_t value) override {
        /** ignored as only input is supported */        
    }

	void writePort(uint8_t pin, uint8_t portVal) override {
        /** ignored as only input is supported */
    }
};

/**
 * Defines the analog ranges to pass to the DfRobotInputAbstraction - default 
 */
const PROGMEM DfRobotAnalogRanges dfRobotAvrRanges { 50, 250, 450, 650, 850};

/**
 * Defines the analog ranges to pass to the DfRobotInputAbstraction - for V1.0 of the board 
 */
const PROGMEM DfRobotAnalogRanges dfRobotV1AvrRanges { 50, 195, 380, 555, 790};

IoAbstractionRef inputFromDfRobotShield(uint8_t pin = A0) {
    return new DfRobotInputAbstraction(&dfRobotAvrRanges, pin);
}

IoAbstractionRef inputFromDfRobotShieldV1(uint8_t pin = A0) {
    return new DfRobotInputAbstraction(&dfRobotV1AvrRanges, pin);
}

#endif
