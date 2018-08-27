/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

/**
 * @file SwitchInput.h
 * 
 * Switch input provides the button and rotary encoder input capabilities provided by this library.
 * There is a globally defined variable `switches` declared that you can use directly. To add a
 * rotary encoder, see the helper functions further down. There's also a rotary encoder emulation
 * based on Up and Down buttons.
 */

#ifndef _SWITCHINPUT_H
#define _SWITCHINPUT_H

#include <IoAbstraction.h>
#include <TaskManager.h>

#ifndef HOLD_THRESHOLD
#define HOLD_THRESHOLD 20
#endif

// START user adjustable section

/**
 * If you want more (or less) buttons, change this definition below to the appropriate number.
 * Each button adds about 6 bytes of RAM, so on a tiny you could adjust downwards for example.
 */
#define MAX_KEYS 4

// END user adjustable section

/** For buttons that should not repeat, and instead just indicate they are HELD down */
#define NO_REPEAT 0xff

enum KeyPressState : byte {
	NOT_PRESSED,
	DEBOUNCING1,
	DEBOUNCING2,
	PRESSED,
	BUTTON_HELD
};

/** 
 * The signature for a callback function that is registered with addSwitch
 * @param key the pin associated with the pin
 * @param heldDown if the button has been held down
 */ 
typedef void(*KeyCallbackFn)(uint8_t key, bool heldDown);

/**
 * The signature of a callback function for rotary encoders, registered when initialising the encoder setupUpDownButtonEncoder
 * @param newValue the value the encoder changed to
 */
typedef void(*EncoderCallbackFn)(int newValue);

/**
 * An internal class that represents the state of a single key being managed by switches.
 */
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
	virtual ~RotaryEncoder() {;}
	void changePrecision(uint16_t maxValue, int currentValue);

	int getCurrentReading() { return currentReading; }
	void setCurrentReading(int reading) { currentReading = reading; }
	void increment(int8_t incVal);
	uint8_t getMenuDivisor() { return menuDivisor; }
	virtual void encoderChanged() {;}
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
	virtual void encoderChanged();
	void debounceAction();
};

class EncoderUpDownButtons : public RotaryEncoder {
public:
	EncoderUpDownButtons(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback, uint8_t speed = 5);
};

#define SW_FLAG_PULLUP_LOGIC 0
#define SW_FLAG_INTERRUPT_DRIVEN 1
#define SW_FLAG_INTERRUPT_DEBOUNCE 2

class SwitchInput {
private:
	RotaryEncoder* encoder;
	IoAbstractionRef ioDevice;
	KeyboardItem keys[MAX_KEYS];
	uint8_t numberOfKeys;
	uint8_t swFlags;
public:
	SwitchInput();
	void initialise(IoAbstractionRef ioDevice, bool usePullUpSwitching = false);
	void initialiseInterrupt(IoAbstractionRef ioDevice, bool usePullUpSwitching = false);
	
	void addSwitch(uint8_t pin, KeyCallbackFn callback, uint8_t repeat = NO_REPEAT);

	void setEncoder(RotaryEncoder* encoder);
	void changeEncoderPrecision(uint16_t precision, uint16_t currentValue);
	uint8_t getMenuDivisor() { return encoder->getMenuDivisor(); }

	bool runLoop();
	IoAbstractionRef getIoAbstraction() { return ioDevice; }
	bool isPullupLogic() {return bitRead(swFlags, SW_FLAG_PULLUP_LOGIC);}
	bool isInterruptDriven() {return bitRead(swFlags, SW_FLAG_INTERRUPT_DRIVEN);}
	bool isInterruptDebouncing() {return bitRead(swFlags, SW_FLAG_INTERRUPT_DEBOUNCE);}
	void setInterruptDebouncing(bool debounce) { bitWrite(swFlags, SW_FLAG_INTERRUPT_DEBOUNCE, debounce);}
private:
	friend void onSwitchesInterrupt(uint8_t);
	friend void switchEncoderUp(uint8_t, bool);
	friend void switchEncoderDown(uint8_t, bool);
};

/**
 * This is the global switch input variable. Do not create other instances of this class.
 */
extern SwitchInput switches;

void setupRotaryEncoderWithInterrupt(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback);
void setupUpDownButtonEncoder(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback);

#endif
