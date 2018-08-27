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

/**
 * Rotary encoder is the base class of both the hardware rotary encoder and the up / down button version. 
 * It handles storing the current value, setting and managing the range of allowed values and calling
 * back when the encoder changes.
 */
class RotaryEncoder {
protected:	
	uint16_t maximumValue;
	uint16_t currentReading;
	EncoderCallbackFn callback;
	uint8_t menuDivisor;
public:
	RotaryEncoder(EncoderCallbackFn callback);
	virtual ~RotaryEncoder() {;}

	/**
	 * Change the precision of the rotary encoder, setting the maximum allowable value and the current value
	 * @param maxValue the largest value allowed
	 * @param currentValue the current value 
	 */
	void changePrecision(uint16_t maxValue, int currentValue);

	/**
	 * Gets the current value of the encoder.
	 */
	int getCurrentReading() { return currentReading; }

	/**
	 * Sets the current value of the encoder.
	 * @param reading will become the new current value.
	 */
	void setCurrentReading(int reading) { currentReading = reading; }

	/**
	 * Change the value represented by the encoder by incVal. Normally called internally.
	 * @param incVal the amount by which to change the encoder.
	 */
	void increment(int8_t incVal);

	/**
	 * Set the divisor used internally, to reduce the sensitivity of the encoder.
	 */
	uint8_t getMenuDivisor() { return menuDivisor; }

	/**
	 * internal method not for external use..
	 */
	virtual void encoderChanged() {;}
};

#define DEBOUNCE_ALAST = 0x01
#define DEBOUNCE_CLEAN = 0x02

/**
 * An implementation of RotaryEncoder that supports the most common types of rotary encoder, needed no additional hardware
 * in most cases. The A input must be an interrupt pin.
 * @see setupRotaryEncoderWithInterrupt
 */  
class HardwareRotaryEncoder : public RotaryEncoder {
private:
	uint8_t pinA;
	uint8_t pinB;
	uint8_t aLast;
	uint8_t cleanFromB;
	
public:
	HardwareRotaryEncoder(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback);
	virtual void encoderChanged();
};

/**
 * An emulation of a rotary encoder using switches for up and down.
 * @see setupUpDownButtonEncoder
 */
class EncoderUpDownButtons : public RotaryEncoder {
public:
	EncoderUpDownButtons(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback, uint8_t speed = 5);
};

#define SW_FLAG_PULLUP_LOGIC 0
#define SW_FLAG_INTERRUPT_DRIVEN 1
#define SW_FLAG_INTERRUPT_DEBOUNCE 2

/**
 * Provides event based switches that are automatically debounced with repeatkey or hold notification.
 * This library integrates with TaskManager and taskManager.runLoop() must therefore be called in the 
 * loop method. This class can handle pull up or pull down switches, either interrupt driven or polling.
 * 
 * Further, this library can work with ANY of the IO abstractions, so the switches can be on either
 * arduino pins or an i2c expander.
 *  
 * @see BasicIoAbstraction
 * @see TaskManager
 */ 
class SwitchInput {
private:
	RotaryEncoder* encoder;
	IoAbstractionRef ioDevice;
	KeyboardItem keys[MAX_KEYS];
	uint8_t numberOfKeys;
	uint8_t swFlags;
public:
	/** 
	 * always use the global switches instance.
	 * @see switches
	 */
	SwitchInput();

	/**
	 * initialise switch input so that it can start managing switches using polling via task manager every 1/20 of a second. If the switches are
	 *  pull the library automatically uses INPUT_PULLUP. For most usages this will mean no external resistors are needed.
	 * @param ioDevice the ioDevice where the switches are connected
	 * @param usePullUpSwitching true if the switches are pull, false for pull down. 
	 */
	void initialise(IoAbstractionRef ioDevice, bool usePullUpSwitching = false);
	/**
	 * initialise switch input so that it can start managing switches using an interrupt to determine switch changes. Polling is only used for
	 * debounce or repeat key actions. If the switches are pull the library automatically uses INPUT_PULLUP. For most usages this will mean no 
	 * external resistors are needed.
	 * @param ioDevice the ioDevice where the switches are connected
	 * @param usePullUpSwitching true if the switches are pull, false for pull down. 
	 */
	void initialiseInterrupt(IoAbstractionRef ioDevice, bool usePullUpSwitching = false);
	
	/**
	 * Add a switch to be managed by switches, it can optionally be a repeat key
	 * @param pin the pin on which the switch is attached
	 * @param callback the function to be called back upon change
	 * @param repeat optional - the frequency in intervals of 1/20th second to repeat.
	 */
	void addSwitch(uint8_t pin, KeyCallbackFn callback, uint8_t repeat = NO_REPEAT);

	/**
	 * Sets the rotary encoder to use, unless you have a custom one, prefer to use the setup methods
	 * @see setupRotaryEncoderWithInterrupt
	 * @see setupUpDownButtonEncoder
	 */
	void setEncoder(RotaryEncoder* encoder);

	/**
	 * This is helper function that calls the rotary encoders change precision function. It changes the
	 * maximum value that can be represented and also the current value of the encoder.
	 * @param precision the maximum value to be set
	 * @param currentValue the current value to be set.
	 */
	void changeEncoderPrecision(uint16_t precision, uint16_t currentValue);
	/**
	 * Helper to get the menu divisor from the encoder, gets the sensitivity of the encoder basically
	 */
	uint8_t getMenuDivisor() { return encoder->getMenuDivisor(); }

	/**
	 * This will normally be called by task manager when not interrupt driven.
	 */
	bool runLoop();

	/** Gets the IoAbstraction that is being used */
	IoAbstractionRef getIoAbstraction() { return ioDevice; }

	/**
	 * Returns true if using pull up button logic, otherwise false.
	 */
	bool isPullupLogic() {return bitRead(swFlags, SW_FLAG_PULLUP_LOGIC);}
	
	/**
	 * Returns true when the library is running in interrupt driven mode - IE not polling 
	 */
	bool isInterruptDriven() {return bitRead(swFlags, SW_FLAG_INTERRUPT_DRIVEN);}
	
	/**
	 * Returns true if in interrupt mode and current attempting a debounce
	 */
	bool isInterruptDebouncing() {return bitRead(swFlags, SW_FLAG_INTERRUPT_DEBOUNCE);}

	/** 
	 * Sets the debounce state - only really for internal use.
	 * @param debounce true if debouncing.
	 */
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

/**
 * Initialise a hardware rotary encoder on the pins passed in, when the value changes the callback function
 * will be called. This library will set pinA and pinB to INPUT_PULLUP, and debounces internally. In most
 * cases no additional components are needed. This function automatically adds the encoder to the global
 * switches instance.
 * @param pinA the first pin of the encoder, this pin must handle interrupts.
 * @param pinB the third pin of the encoder, the middle pin goes to ground.
 * @param callback the function that will receive the new state of the encoder on changes.
 */
void setupRotaryEncoderWithInterrupt(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback);
/**
 * Initialise an encoder that uses up and down buttons to handle the same functions as a hardware encoder.
 * This function automatically adds the encoder to the global switches instance.
 * @param pinUp the up button
 * @param pinDown the down button
 * @param callback the function that will receive the new state on change.
 */
void setupUpDownButtonEncoder(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback);

#endif
