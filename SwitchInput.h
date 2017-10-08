/*
SwitchInput library for Arduino
Released under Apache V2 license by www.thecoderscorner.com (Dave Cherry).

This library provides a callback interface for switch / encoder based inputs
*/

#ifndef _SWITCHINPUT_H
#define _SWITCHINPUT_H

#include <BasicIoAbstraction.h>

#ifndef HOLD_THRESHOLD
#define HOLD_THRESHOLD 20
#endif

// START user adjustable section

// If you want more (or less) than 5 buttons, change this definition below to the appropriate number.
// Each button adds about 4 bytes of RAM, so on a tiny you could adjust downwards for example.
#define MAX_KEYS 5

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

class KeyboardItem {
private:
	KeyPressState state;
	uint8_t pin;
	uint8_t counter;
	uint8_t repeatInterval;
public:
	KeyboardItem();

	void initialise(uint8_t pin, uint8_t repeatInterval = NO_REPEAT);
	void checkAndTrigger(uint8_t pin, KeyCallbackFn callback);

	bool isDebouncing() { return state == DEBOUNCING1 || state == DEBOUNCING2; }
	bool isPressed() { return state == PRESSED || state == BUTTON_HELD; }
	bool isHeld() { return state == BUTTON_HELD; }
	uint8_t getPin() { return pin;  }
};

class RotaryEncoder {
private:
	uint8_t pinA;
	uint8_t pinB;
	uint8_t precision;
	uint8_t lastStates;
	int currentReading;

public:
	RotaryEncoder();
	void initialise(uint8_t pinA, uint8_t pinB, uint8_t precision);

	int getCurrentReading() { return currentReading / precision; }
	void setCurrentReading(int reading) { currentReading = reading * precision; }
	bool isInitialised() { pinA != 0xff; }
};

class SwitchInput {
private:
	RotaryEncoder encoder;
	KeyboardItem keys[MAX_KEYS];
	uint8_t numberOfKeys;
	KeyCallbackFn keyCallback;
	BasicIoAbstraction* ioDevice;

public:
	SwitchInput(BasicIoAbstraction* facilities);

	void addSwitch(uint8_t pin, uint8_t repeat = NO_REPEAT);
	void addRotataryEncoder(uint8_t pinA, uint8_t pinB, uint8_t precision);

	void initialise(KeyCallbackFn callback) { keyCallback = callback; }

	void runLoop();
};

#endif