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
#ifndef MAX_KEYS
#define MAX_KEYS 5
#endif // MAX_KEYS defined

/** 
 * If you want to adjust the maximum number of rotary encoders from the default of 4 just either
 * change the definition below or set this define during compilation.
 */
#ifndef MAX_ROTARY_ENCODERS
#define MAX_ROTARY_ENCODERS 4
#endif // MAX_ROTARY_ENCODERS


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

#define KEY_PRESS_STATE_MASK 0x0f
#define KEY_LISTENER_MODE_BIT 7

/**
 * Used to register a class that has an interest in the state of a switch.
 * Implement the two virtual functions that will be called back
 * instead of the KeyCallbackFn callback function. This is generally passed
 * addSwitch(...) and the onPressed / onReleased methods are called upon
 * each event.
 */
class SwitchListener {
public:
	/**
	 * called when a key is pressed or held down
	 * @param pin the pin number
	 * @param held true if held down
	 */
	virtual void onPressed(uint8_t pin, bool held) = 0;
	/**
	 * called when a key is released
	 * @param pin the key number
	 * @param held true if key was held down
	 */
	virtual void onReleased(uint8_t pin, bool held) = 0;
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
	uint8_t stateFlags;
	KeyPressState previousState;
	uint8_t pin;
	uint8_t counter;
	uint8_t acceleration;
	uint8_t repeatInterval;
	union {
		KeyCallbackFn callback;
		SwitchListener* listener;
	} notify;
	KeyCallbackFn callbackOnRelease;
public:
	KeyboardItem();

	void initialise(uint8_t pin, KeyCallbackFn callback, uint8_t repeatInterval = NO_REPEAT);
	void initialise(uint8_t pin, SwitchListener* switchListener, uint8_t repeatInterval = NO_REPEAT);
	void checkAndTrigger(uint8_t pin);
	void onRelease(KeyCallbackFn callbackOnRelease);

	bool isDebouncing() { return getState() == DEBOUNCING1 || getState() == DEBOUNCING2; }
	bool isPressed() { return getState() == PRESSED || getState() == BUTTON_HELD; }
	bool isHeld() { return getState() == BUTTON_HELD; }
	uint8_t getPin() { return pin;  }
	
	void trigger(bool held);
	void triggerRelease(bool held);

	KeyPressState getState() { return (KeyPressState)(stateFlags & KEY_PRESS_STATE_MASK); }
	void setState(KeyPressState state) { 
		stateFlags &= ~KEY_PRESS_STATE_MASK; 
		stateFlags |= (state & KEY_PRESS_STATE_MASK);
	}
	bool isUsingListener() { return bitRead(stateFlags, KEY_LISTENER_MODE_BIT);  }
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
	unsigned long lastChange;
	uint8_t pinA;
	uint8_t pinB;
	uint8_t aLast;
	uint8_t cleanFromB;
	
public:
	HardwareRotaryEncoder(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback);
	virtual void encoderChanged();
private:
	int amountFromChange(unsigned long change);
};

/**
 * An emulation of a rotary encoder using switches for up and down.
 * @see setupUpDownButtonEncoder
 */
class EncoderUpDownButtons : public RotaryEncoder {
public:
	EncoderUpDownButtons(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback, uint8_t speed = 20);
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
	RotaryEncoder* encoder[MAX_ROTARY_ENCODERS];
	IoAbstractionRef ioDevice;
	KeyboardItem keys[MAX_KEYS];
	uint8_t numberOfKeys;
	volatile uint8_t swFlags;
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
	 * @return true if successful, false if the pin could not be registered.
	 */
	bool addSwitch(uint8_t pin, KeyCallbackFn callback, uint8_t repeat = NO_REPEAT);

	/**
	 * Add a switch to be managed by switches using an implementation of the
	 * SwitchListener interface to receive events instead of function callbacks.
	 * @param pin the pin on which the switch is attached
	 * @param listener reference to a listener that will be notified of press and release.
	 * @param repeat optional - the frequency in intervals of 1/20th second to repeat.
	 * @return true if successful, false if the pin could not be registered.
	 */
	bool addSwitchListener(uint8_t pin, SwitchListener* listener, uint8_t repeat = NO_REPEAT);

	/**
	 * Set callback the function to be called back upon key release
	 * @param pin the pin on which the switch is attached
	 * @param callbackOnRelease the function to be called back upon key release
	 */
	void onRelease(uint8_t pin, KeyCallbackFn callbackOnRelease);

	/**
	 * Sets the rotary encoder to use, unless you have a custom one, prefer to use the setup methods
	 * @see setupRotaryEncoderWithInterrupt
	 * @see setupUpDownButtonEncoder
	 */
	void setEncoder(RotaryEncoder* encoder) { this->encoder[0] = encoder; };

	/**
	 * Use this method if you want to work with serveral encoders. This lib defaults to 4 (value of MAX_ROTARY_ENCODERS)
	 * encoders, but the actual number of encoders depends on the hardware you are using and the value of that define. If your port 
	 * expander is 8-bit it supports up to 4 rotary encoders. To use more than 4, you set MAX_ROTARY_ENCODERS to the value you need and
	 * use a 16-bit encoder.
	 * @param slot the index of the encoder to set, zero based.
	 * @param encoder the encoder to be added.
	 */
	void setEncoder(uint8_t slot, RotaryEncoder* encoder);

	/**
	 * Gets a pointer to the current encoder, or NULL if there is not one
	 */
	RotaryEncoder* getEncoder() {return encoder[0]; }

	/**
	 * This is helper function that calls the rotary encoders change precision function. It changes the
	 * maximum value that can be represented and also the current value of the encoder.
	 * @param precision the maximum value to be set
	 * @param currentValue the current value to be set.
	 */
	void changeEncoderPrecision(uint16_t precision, uint16_t currentValue) { changeEncoderPrecision(0, precision, currentValue); }

	/**
	 * Use this version of changeEncoderPrecision if you are working with more than one rotary encoder.
	 * This is helper function that calls the rotary encoders change precision function. It changes the
	 * maximum value that can be represented and also the current value of the encoder.
	 * @param slot the index of the desired encoder, zero based
	 * @param precision the maximum value to be set
	 * @param currentValue the current value to be set.
	 */
	void changeEncoderPrecision(uint8_t slot, uint16_t precision, uint16_t currentValue);

	/**
	 * Simulates a switch press by calling the callback directly without changing the internal state
	 * of the key. Useful to simulate a key press in some situations.
	 * @param pin the pin associated with the switch
	 * @param held if the held state should be set on the callback
	 */
	void pushSwitch(uint8_t pin, bool held);

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
	 * Returns true if the switch at the defined pin is pressed, otherwise false
	 * @param pin the pin to check if pressed
	 */
	bool isSwitchPressed(uint8_t pin);

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
