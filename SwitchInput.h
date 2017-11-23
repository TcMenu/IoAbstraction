/*
SwitchInput library for Arduino
Released under Apache V2 license by www.thecoderscorner.com (Dave Cherry).

This library provides a callback interface for switch / encoder based inputs
*/

#ifndef _SWITCHINPUT_H
#define _SWITCHINPUT_H

#include <BasicIoAbstraction.h>
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

#define ROTARY_PRECISION_65355 0
#define ROTARY_PRECISION_32768 1
#define ROTARY_PRECISION_16348 2
#define ROTARY_PRECISION_4096 4
#define ROTARY_PRECISION_1024 6
#define ROTARY_PRECISION_256 8

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
private:
	uint8_t pinA;
	uint8_t pinB;
	uint16_t maximumValue;
	uint16_t currentReading;
	EncoderCallbackFn callback;
public:
	RotaryEncoder();
	void initialise(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback);
	void changePrecision(uint16_t maxValue, int currentValue);

	int getCurrentReading() { return currentReading; }
	void setCurrentReading(int reading) { currentReading = reading; }
	bool isInitialised() { pinA != 0xff; }
	void encoderChanged(IoAbstractionRef ioDevice);
};

void onEncoderInterrupt();

class SwitchInput {
private:
	RotaryEncoder encoder;
	KeyboardItem keys[MAX_KEYS];
	uint8_t numberOfKeys;
	IoAbstractionRef ioDevice;
public:
	SwitchInput();
	void initialise(TaskManager& taskManager, IoAbstractionRef ioDevice);
	
	void addSwitch(uint8_t pin, KeyCallbackFn callback, uint8_t repeat = NO_REPEAT);

	void initialiseEncoder(uint8_t pinA, uint8_t pinB, uint8_t precision, EncoderCallbackFn callback);
	void changeEncoderPrecision(uint16_t precision, uint16_t currentValue);
	void encoderChanged();

	void runLoop();
private:
	static SwitchInput* __swInInstance;
	friend void onEncoderInterrupt();
};

#endif