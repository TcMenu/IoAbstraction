/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _SWITCHINPUT_H
#define _SWITCHINPUT_H

#include <IoAbstraction.h>
#include <TaskManager.h>

#ifndef HOLD_THRESHOLD
#define HOLD_THRESHOLD 20
#endif

// START user adjustable section

// If you want more (or less) than 5 buttons, change this definition below to the appropriate number.
// Each button adds about 6 bytes of RAM, so on a tiny you could adjust downwards for example.
#define MAX_KEYS 4

// END user adjustable section

#define NO_REPEAT 0xff

enum KeyPressState : byte {
	NOT_PRESSED,
	DEBOUNCING1,
	DEBOUNCING2,
	PRESSED,
	BUTTON_HELD
};

typedef void(*KeyCallbackFn)(uint8_t key, bool heldDown);
typedef void(*EncoderCallbackFn)(int newValue);

class KeyboardItem {
private:
	KeyPressState state;
	uint8_t pin;
	uint8_t counter;
	uint8_t repeatInterval;
	KeyCallbackFn callback;
public:
	KeyboardItem();

	void initialise(uint8_t pin, KeyCallbackFn callback, uint8_t repeatInterval = NO_REPEAT);
	void checkAndTrigger(uint8_t pin);

	bool isDebouncing() { return state == DEBOUNCING1 || state == DEBOUNCING2; }
	bool isPressed() { return state == PRESSED || state == BUTTON_HELD; }
	bool isHeld() { return state == BUTTON_HELD; }
	uint8_t getPin() { return pin;  }
};

class RotaryEncoder {
protected:	
	uint16_t maximumValue;
	uint16_t currentReading;
	EncoderCallbackFn callback;
	uint8_t menuDivisor;
public:
	RotaryEncoder(EncoderCallbackFn callback);
	void changePrecision(uint16_t maxValue, int currentValue);

	int getCurrentReading() { return currentReading; }
	void setCurrentReading(int reading) { currentReading = reading; }
	void increment(int8_t incVal);
	uint8_t getMenuDivisor() { return menuDivisor; }
};

#define DEBOUNCE_ALAST = 0x01
#define DEBOUNCE_CLEAN = 0x02

class HardwareRotaryEncoder : public RotaryEncoder {
private:
	uint8_t pinA;
	uint8_t pinB;
	uint8_t aLast;
	uint8_t cleanFromB;
	
public:
	HardwareRotaryEncoder(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback);
	void encoderChanged();
	void debounceAction();
};

class EncoderUpDownButtons : public RotaryEncoder {
public:
	EncoderUpDownButtons(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback, uint8_t speed = 5);
};

class SwitchInput {
private:
	RotaryEncoder* encoder;
	KeyboardItem keys[MAX_KEYS];
	uint8_t numberOfKeys;
	IoAbstractionRef ioDevice;
	bool pinsArePullUp;
public:
	SwitchInput();
	void initialise(IoAbstractionRef ioDevice, bool usePullUpSwitching = false);
	
	void addSwitch(uint8_t pin, KeyCallbackFn callback, uint8_t repeat = NO_REPEAT);

	void setEncoder(RotaryEncoder* encoder);
	void changeEncoderPrecision(uint16_t precision, uint16_t currentValue);
	uint8_t getMenuDivisor() { return encoder->getMenuDivisor(); }

	void runLoop();
	
	IoAbstractionRef getIoAbstraction() { return ioDevice; }
private:
	friend void onEncoderInterrupt(uint8_t);
	friend void switchEncoderUp(uint8_t, bool);
	friend void switchEncoderDown(uint8_t, bool);
};

extern SwitchInput switches;

void setupRotaryEncoderWithInterrupt(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback);
void setupUpDownButtonEncoder(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback);

#endif
