/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

/**
 * @file SwitchInput.h
 * @brief Switch input provides the button and rotary encoder input capabilities provided by this library.
 * There is a globally defined variable `switches` declared that you can use directly. To add a
 * rotary encoder, see the helper functions further down. There's also a rotary encoder emulation
 * based on Up and Down buttons.
 */

#ifndef _SWITCHINPUT_H
#define _SWITCHINPUT_H

#include <IoAbstraction.h>
#include <TaskManager.h>
#include <SimpleCollections.h>

// START user adjustable section

// The threshold for an item becoming held down or for it to repeat, this is about half a second by default
#ifndef HOLD_THRESHOLD
#define HOLD_THRESHOLD 20
#endif //HOLD_THRESHOLD

// The rate a which switches will speed up under acceleration, 0 = extremely fast (untested!), 1 = fast, 2 = regular, 3 = slower
#ifndef SWITCHES_ACCELERATION_DIVISOR
#define SWITCHES_ACCELERATION_DIVISOR 2
#endif //SWITCHES_ACCELERATION_DIVISOR

/*
 * If you want more buttons, the library will reallocate as needed, however this is not efficient and in production
 * probably better to set this value to about the number of switches needed.  Each button adds about 10 bytes of RAM,
 * so on a tiny you could adjust downwards for example.
 */
#ifndef MAX_KEYS
#define MAX_KEYS DEFAULT_LIST_SIZE
#endif // MAX_KEYS defined

/**
 * If you want to adjust the maximum number of rotary encoders from the default of 4 just either
 * change the definition below or set this define during compilation.
 */
#ifndef MAX_ROTARY_ENCODERS
#define MAX_ROTARY_ENCODERS 4
#endif // MAX_ROTARY_ENCODERS

/*
 * This parameter defines the interval between polling on switches. It defaults to 20 millis per poll.
 */
#ifndef SWITCH_POLL_INTERVAL
#define SWITCH_POLL_INTERVAL 20
#endif // SWITCH_POLL_INTERVAL

/*
 * This parameter defines the time threshold for which the rotary encoder should reject a direction change as
 * part of the debouncing. IE if there is a spike that would represent a "valid" direction change this would prevent
 * that direction change if within 100 milliseconds of a turn in the other direction. A user would have to be pretty
 * quick to change direction in 1/10th of a second, but it is configurable in case.
 */
#ifndef REJECT_DIRECTION_CHANGE_THRESHOLD
#define REJECT_DIRECTION_CHANGE_THRESHOLD 10000
#endif //REJECT_DIRECTION_CHANGE_THRESHOLD

// END user adjustable section

/** For buttons that should not repeat, and instead just indicate they are HELD down */
#define NO_REPEAT 0xff

enum KeyPressState : uint8_t {
	NOT_PRESSED,
	DEBOUNCING1,
	DEBOUNCING2,
	PRESSED,
	BUTTON_HELD
};

#define KEY_PRESS_STATE_MASK 0x0f
#define KEY_LISTENER_MODE_BIT 7
#define KEY_LOGIC_IS_INVERTED 6

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
	virtual void onPressed(pinid_t pin, bool held) = 0;
	/**
	 * called when a key is released
	 * @param pin the key number
	 * @param held true if key was held down
	 */
	virtual void onReleased(pinid_t pin, bool held) = 0;
};

/** 
 * The signature for a callback function that is registered with addSwitch, you can also implement the SwitchListener
 * interface instead to receive updates.
 * @param key the pin associated with the pin
 * @param heldDown if the button has been held down
 */
typedef void(*KeyCallbackFn)(pinid_t key, bool heldDown);

/**
 * The signature for the encoder callback, in addition to this, you can also implement the EncoderListener interface
 * instead to receive updates.
 * @param newValue the value of the rotary encoder
 */
typedef void(*EncoderCallbackFn)(int newValue);

/**
 * An internal class that represents the state of a single key being managed by switches.
 */
class KeyboardItem {
private:
	uint8_t stateFlags;
	KeyPressState previousState;
	pinid_t pin;
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
    KeyboardItem(pinid_t pin, KeyCallbackFn callback, uint8_t repeatInterval = NO_REPEAT, bool keyLogicIsInverted = false);
    KeyboardItem(pinid_t pin, SwitchListener* switchListener, uint8_t repeatInterval = NO_REPEAT, bool keyLogicIsInverted = false);
    KeyboardItem(const KeyboardItem& other);
    KeyboardItem& operator=(const KeyboardItem& other);
	void checkAndTrigger(uint8_t pin);
	void onRelease(KeyCallbackFn callbackOnRelease);

	bool isDebouncing() const { return getState() == DEBOUNCING1 || getState() == DEBOUNCING2; }
	bool isPressed() const { return getState() == PRESSED || getState() == BUTTON_HELD; }
	bool isHeld() const { return getState() == BUTTON_HELD; }
	pinid_t getPin() const { return pin;  }
	pinid_t getKey() const { return pin; }
	
	void trigger(bool held);
	void triggerRelease(bool held);

	KeyPressState getState() const { return (KeyPressState)(stateFlags & KEY_PRESS_STATE_MASK); }
	void setState(KeyPressState state) { 
		stateFlags &= ~KEY_PRESS_STATE_MASK; 
		stateFlags |= (state & KEY_PRESS_STATE_MASK);
	}
	bool isUsingListener() { return bitRead(stateFlags, KEY_LISTENER_MODE_BIT); }
	bool isLogicInverted() { return bitRead(stateFlags, KEY_LOGIC_IS_INVERTED); }

	void changeOnPressed(KeyCallbackFn pFunction);
	void changeListener(SwitchListener* listener);
};

/**
 * When working with rotary encoders there's three possible ways that the user will interact, and it is this
 * intent that we need to capture, they are either using it for direction only, to scroll through items,
 * or to change a value.
 */
enum EncoderUserIntention: uint8_t {
    /** User wishes to change or set a value */
    CHANGE_VALUE = 0,
    /** User wishes to scroll through a list of items */
    SCROLL_THROUGH_ITEMS,
    /** User wishes to scrool through items sideways, such as card layout. */
    SCROLL_THROUGH_SIDEWAYS,
    /** User is just using the encoder for direction only */
    DIRECTION_ONLY
};

/**
 * Optionally you can extend your class from EncoderListener and then receive the callbacks in the encoderHasChanged
 * method instead of using a functional callback.
 */
class EncoderListener {
public:
    /**
     * Called when the encoder has changed to a new value, you must implement this method in the class that extends from
     * EncoderListener.
     * @param newValue the new value of the encoder.
     */
    virtual void encoderHasChanged(int newValue)=0;
};

/**
 * Rotary encoder is the base class of both the hardware rotary encoder and the up / down button version. 
 * It handles storing the current value, setting and managing the range of allowed values and calling
 * back when the encoder changes.
 */
class RotaryEncoder {
protected:
    enum EncoderFlagBits { LAST_SYNC_STATUS=0, WRAP_AROUND_MODE, OO_LISTENER_CALLBACK, LAST_ENCODER_DIRECTION_UP };
	uint16_t maximumValue;
	uint16_t currentReading;
    uint8_t stepSize;
    union {
        EncoderCallbackFn callback;
        EncoderListener* encoderListener;
    } notify;
    uint8_t flags;
    EncoderUserIntention intent;
public:
	explicit RotaryEncoder(EncoderCallbackFn callback);
    explicit RotaryEncoder(EncoderListener* listener);
    virtual ~RotaryEncoder() {;}

	/**
	 * Change the precision of the rotary encoder, setting the maximum allowable value and the current value. If you set the maximum value
     * to a positive value, then the encoder will work like a potentiometer between 0 and the maximum value. However, if you set maximum to
     * zero, then the encoder works in direction mode, where you'll get called back with either 0-unchanged, 1-up, -1-down.
	 * @param maxValue the largest value allowed or zero for direction only mode
	 * @param currentValue the current value (zero for direction mode)
	 */
	void changePrecision(uint16_t maxValue, int currentValue, bool rolloverOnMax = false, int step = 1);

	/**
	 * Change the callback that will be used to notify of changes in the encoder value, this must never be null.
	 * @param callbackFn the new callback function
	 */
	void replaceCallback(EncoderCallbackFn callbackFn);

    /**
     * Change the callback that will be used to notify of changes in the encoder value, this must never be null.
     * @param callbackFn the new callback function
     */
    void replaceCallbackListener(EncoderListener* callbackFn);

	/**
	 * Gets the current value of the encoder.
	 */
	int getCurrentReading() const { return currentReading; }

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

    /**
     * Used to get the last sync status of the underlying IoAbstraction. Useful when working
     * with devices over i2c to check if the comms worked.
     * @return true if the sync was successful, otherwise.
     */
    bool didLastSyncSucceed() { return bitRead(flags, LAST_SYNC_STATUS); }

    /**
     * For joystick and up/down button encoders there is a difference between scroll using
     * the encoder, and presenting the menu using the encoder, unlike rotary encoders where
     * both modes feel natural the same way, there is a need to invert scrolling on button
     * and joysticks.
     */
    void setUserIntention(EncoderUserIntention intention);

    EncoderUserIntention getUserIntention() { return intent; }

    void runCallback(int newVal) {
        if(bitRead(flags, OO_LISTENER_CALLBACK)) {
            notify.encoderListener->encoderHasChanged(newVal);
        } else {
            notify.callback(newVal);
        }
    }

    /**
     * @return the maximum value that this encoder can be set to.
     */
    bsize_t getMaximumValue() { return maximumValue; }
};

/**
 * This enumeration is used to control how acceleration is handled within a particular instance
 * of a HardwareRotaryEncoder.
 */
enum HWAccelerationMode : uint8_t {
    /** No acceleration, no matter how fast the encoder is turned */
    HWACCEL_NONE,
    /** The default, accelerates based on how fast the encoder is turned */
    HWACCEL_REGULAR,
    /** Slower acceleration than above is applied */ 
    HWACCEL_SLOWER
};

/**
 * This enumeration is used to define how an encoder's detents relate to
 * its output states
 */
enum EncoderType : uint8_t {
    /** Detent after every signal change, A or B */
    QUARTER_CYCLE,
    /** Detent on every position where A == B */
    HALF_CYCLE,
    /** Detent after every full cycle of both signals, A and B */ 
    FULL_CYCLE
};

class AbstractHwRotaryEncoder : public RotaryEncoder {
protected:
    unsigned long lastChange;
    pinid_t pinA;
    pinid_t pinB;
    HWAccelerationMode accelerationMode;
    EncoderType encoderType;

public:
    explicit AbstractHwRotaryEncoder(EncoderCallbackFn callback) : RotaryEncoder(callback) {}
    explicit AbstractHwRotaryEncoder(EncoderListener* listener) : RotaryEncoder(listener) {}

    /**
     * Allows for changes in the acceleration mode at runtime
     * @param mode the new acceleration mode
     */
    void setAccelerationMode(HWAccelerationMode mode) { accelerationMode =  mode; }

    /**
     * Allows for changes in encoder type at runtime
     * @param encoderType change in encoder type
     */
    void setEncoderType(EncoderType et) { encoderType =  et; }

protected:
    void initialiseBase(pinid_t pinA, pinid_t pinB, HWAccelerationMode accelerationMode, EncoderType);
    int amountFromChange(unsigned long change);
    void handleChangeRaw(bool increase);
};

/**
 * An implementation of RotaryEncoder that supports the most common types of rotary encoder, needed no additional hardware
 * in most cases. For single encoders registered with switches see the helper method. This is now only used by switches
 * helper functions when legacy mode is enabled using TC_LEGACY_ENCODER.
 * For all new cases we recommend using HwStateRotaryEncoder instead.
 * @see setupRotaryEncoderWithInterrupt
 */  
class HardwareRotaryEncoder : public AbstractHwRotaryEncoder {
private:
    uint8_t state = 0;        // Current state of the encoder
    uint8_t pulseCounter = 0; // Pulse counter for FULL_CYCLE and HALF_CYCLE modes
public:
    /**
     * Create an instance of a hardware rotary encoder specifying the A and B pin, the acceleration parameters and encoder type.
     * It is your responsibility to register this encoder with switches using setEncoder(n, encoderPtr) if you use the constructor.
     * @param pinA the A pin of the encoder
     * @param pinB the B pin of the encoder
     * @param callback the function callback to be called when triggered
     * @param accelerationMode the amount of acceleration to use
     */
	HardwareRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderCallbackFn callback, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType = FULL_CYCLE);

    /**
     * Create an instance of a hardware rotary encoder specifiying the A and B pin, the acceleration parameters and encoder type.
     * It is your responsibility to register this encoder with switches using setEncoder(n, encoderPtr) if you use the constructor.
     * This constructor takes an OO listener instead of a callback function, the listener implements EncoderListener.
     * @param pinA the A pin of the encoder
     * @param pinB the B pin of the encoder
     * @param listener the OO listener extending from EncoderListener
     * @param accelerationMode the amount of acceleration to use
     */
	HardwareRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderListener* listener, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType = FULL_CYCLE);
	void encoderChanged() override;
private:
    void initialise(pinid_t pinA, pinid_t pinB, HWAccelerationMode accelerationMode, EncoderType et);
};

/**
 * An implementation of RotaryEncoder that supports the most common types of rotary encoder, needed no additional hardware
 * in most cases. It is based on a state machine of valid possible states, and should handle quarter turn encoders properly.
 * Interrupt mode is not mandatory, but be aware that without interrupts the polling is approximately every 2 milliseconds.
 * @see setupRotaryEncoderWithInterrupt
 */
class HwStateRotaryEncoder : public AbstractHwRotaryEncoder {
private:
    int8_t currentEncoderState = 0;
public:
    /**
     * Create an instance of a hardware rotary encoder specifying the A and B pin, the acceleration parameters and encoder type.
     * It is your responsibility to register this encoder with switches using setEncoder(n, encoderPtr) if you use the constructor.
     * @param pinA the A pin of the encoder
     * @param pinB the B pin of the encoder
     * @param callback the function callback to be called when triggered
     * @param accelerationMode the amount of acceleration to use
     */
    HwStateRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderCallbackFn callback, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType = FULL_CYCLE);

    /**
     * Create an instance of a hardware rotary encoder specifiying the A and B pin, the acceleration parameters and encoder type.
     * It is your responsibility to register this encoder with switches using setEncoder(n, encoderPtr) if you use the constructor.
     * This constructor takes an OO listener instead of a callback function, the listener implements EncoderListener.
     * @param pinA the A pin of the encoder
     * @param pinB the B pin of the encoder
     * @param listener the OO listener extending from EncoderListener
     * @param accelerationMode the amount of acceleration to use
     */
    HwStateRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderListener* listener, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType = FULL_CYCLE);

    void encoderChanged() override;
private:
    int8_t stateFor(uint8_t bits);
};


/**
 * An emulation of a rotary encoder using switches for up and down.
 * @see setupUpDownButtonEncoder
 */
class EncoderUpDownButtons : public RotaryEncoder, public SwitchListener {
private:
    const pinid_t upPin, downPin, backPin, nextPin;
    SwitchListener* passThroughListener;
    const bool canRotate;
public:
    /**
     * Create an up down encoder based on two buttons, for up and down. This is a helper that wraps calls to switches
     * and handles the up and down events. This version takes a function callback for change notification.
     * @param pinUp the pin to use for up
     * @param pinDown the pin to use for down
     * @param callback the function callback when the encoder changes
     * @param speed the speed of repeat functions on the keys
     */
	EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, EncoderCallbackFn callback, uint8_t speed = 20);

    /**
     * Create an up down encoder based on two buttons, for up and down. This is a helper that wraps calls to switches
     * and handles the up and down events. This version takes an OO listener for change notification.
     * @param pinUp the pin to use for up
     * @param pinDown the pin to use for down
     * @param callback the function callback when the encoder changes
     * @param speed the speed of repeat functions on the keys
     */
	EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, EncoderListener* listener, uint8_t speed = 20);

    /**
     * Create an up down encoder based on two buttons, for up and down. This is a helper that wraps calls to switches
     * and handles the up and down events. This version takes a function callback for change notification. Next and back
     * are passed through to the passthrough callback so you can processes these commands. Calling the rotate meaning
     * turns the encoder so that next and back function as up and down instead of the regular function keys. However,
     * for the passthrough callback the original next and back pin numbers are sent, so outside of the encoder, no
     * extra logic is needed.
     * @param pinUp the pin to use for up
     * @param pinDown the pin to use for down
     * @param pinBack the pin to use for back
     * @param pinNext the pin to use for next
     * @param passThrough the passthrough listener on which this class will convey next and back key presses
     * @param callback the function callback when the encoder changes
     * @param speed the speed of repeat functions on the keys
     */
    EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, pinid_t pinBack, pinid_t pinNext, SwitchListener* passThrough, EncoderCallbackFn callback, uint8_t speed = 20);

    /**
     * Create an up down encoder based on four buttons, for up and down. This is a helper that wraps calls to switches
     * and handles the up and down events. This version takes an OO listener for change notification. The next and back
     * are passed through to the passthrough callback so you can processes these commands. Calling the rotate meaning
     * turns the encoder so that next and back function as up and down instead of the regular function keys. However,
     * for the passthrough callback the original next and back pin numbers are sent, so outside of the encoder, no
     * extra logic is needed.
     * @param pinUp the pin to use for up
     * @param pinDown the pin to use for down
     * @param pinBack the pin to use for back
     * @param pinNext the pin to use for next
     * @param passThrough the passthrough listener on which this class will convey next and back key presses
     * @param listener the OO interface implementation for when encoder changes
     * @param speed the speed of repeat functions on the keys
     */
    EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, pinid_t pinBack, pinid_t pinNext, SwitchListener* passThrough, EncoderListener* listener, uint8_t speed = 20);

    void onPressed(pinid_t pin, bool held) override;
    void onReleased(pinid_t pin, bool held) override;

protected:
    pinid_t getIncrementPin() { return intent == SCROLL_THROUGH_SIDEWAYS && canRotate ? nextPin : upPin; }
    pinid_t getDecrementPin() { return intent == SCROLL_THROUGH_SIDEWAYS && canRotate ? backPin : downPin; }
    pinid_t getBackPin() { return intent == SCROLL_THROUGH_SIDEWAYS && canRotate ? upPin : backPin; }
    pinid_t getNextPin() { return intent == SCROLL_THROUGH_SIDEWAYS && canRotate ? downPin : nextPin; }
};

#define SW_FLAG_PULLUP_LOGIC 0
#define SW_FLAG_INTERRUPT_DRIVEN 1
#define SW_FLAG_INTERRUPT_DEBOUNCE 2
#define SW_FLAG_ENCODER_IS_POLLING 3

/**
 * An enumeration of values, one of which is used when calling switches.init to tell switches what to poll for, or
 * not to poll at all.
 */
enum SwitchInterruptMode {
    /** Do no polling, everything is interrupt driven, it doesn't matter if the interrupt is shared over many keys */
    SWITCHES_NO_POLLING,
    /** Poll for keys but the rotary encoder is managed by interrupt */
    SWITCHES_POLL_KEYS_ONLY,
    /** Poll for everything, there are no interrupts defined in this mode. Halves the switch poll interval.  */
    SWITCHES_POLL_EVERYTHING
};


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
	BtreeList<pinid_t, KeyboardItem> keys;
	volatile uint8_t swFlags;
    bool lastSyncStatus;
public:
	/**
	 * always use the global switches instance.
	 * @see switches
	 */
	explicit SwitchInput();

	/**
	 * initialise switch input so that it can start managing switches using polling via task manager every 1/20 of a second. If the switches are
	 *  pull the library automatically uses INPUT_PULLUP. For most usages this will mean no external resistors are needed.
	 * @param ioDevice the ioDevice where the switches are connected
	 * @param usePullUpSwitching true if the switches are pull, false for pull down.
	 * @deprecated please use init(IoAbstractionRef, SwitchInterruptMode) instead in new code
	 */
	void initialise(IoAbstractionRef ioDevice, bool usePullUpSwitching = false);

    /**
     * initialise switch input so that it can start managing switches either using polling via task manager or via an
     * interrupt callback if so configured. Note that the configuration of `mode` defines the polling, and when set
     * to NO_POLLING it means that switches completely relies on interrupts.
     *
     * You can define the default for pull up / pull down, and if the code is interrupt driven. If the switches
     * are in pull-up mode the library automatically uses INPUT_PULLUP. At least for testing, this will mean no external
     * resistors are needed. Prefer this in newer code to initialise which is kept around for backward compatibility.
     *
     * Switches and the encoder will use the same `ioDevice` that will be used to register any needed interrupts, set
     * up pin direction
     *
     * @param ioDevice the ioDevice where the switches are connected
     * @param interruptMode mode the interrupt mode to operate in
     * @param defaultIsPullUp if true the default is pull up, otherwise the default is pull down
     */
    void init(IoAbstractionRef ioDevice, SwitchInterruptMode mode, bool defaultIsPullUp);

	/**
	 * initialise switch input so that it can start managing switches using an interrupt to determine switch changes. Polling is only used for
	 * debounce or repeat key actions. If the switches are pull the library automatically uses INPUT_PULLUP. For most usages this will mean no 
	 * external resistors are needed.
	 * @param ioDevice the ioDevice where the switches are connected
	 * @param usePullUpSwitching true if the switches are pull, false for pull down. 
	 * @deprecated please use init(IoAbstractionRef, SwitchInterruptMode) instead in new code
	 */
	void initialiseInterrupt(IoAbstractionRef ioDevice, bool usePullUpSwitching = false);
	
	/**
	 * Add a switch to be managed by switches, it can optionally be a repeat key
	 * @param pin the pin on which the switch is attached
	 * @param callback the function to be called back upon change
	 * @param invertLogic optional - inverts the logic between active high and active low for one switch
	 * @param repeat optional - the frequency in intervals of 1/20th second to repeat.
	 * @return true if successful, false if the pin could not be registered.
	 */
	bool addSwitch(pinid_t pin, KeyCallbackFn callback, uint8_t repeat = NO_REPEAT, bool invertLogic = false);

	/**
	 * Add a switch to be managed by switches using an implementation of the
	 * SwitchListener interface to receive events instead of function callbacks.
	 * @param pin the pin on which the switch is attached
	 * @param listener reference to a listener that will be notified of press and release.
	 * @param invertLogic optional - inverts the logic between active high and active low for one switch
	 * @param repeat optional - the frequency in intervals of 1/20th second to repeat.
	 * @return true if successful, false if the pin could not be registered.
	 */
	bool addSwitchListener(pinid_t pin, SwitchListener* listener, uint8_t repeat = NO_REPEAT, bool invertLogic = false);

	/**
	 * Set callback the function to be called back upon key release
	 * @param pin the pin on which the switch is attached
	 * @param callbackOnRelease the function to be called back upon key release
	 */
	void onRelease(pinid_t pin, KeyCallbackFn callbackOnRelease);

	/**
	 * Allows for a change of the function to be called when a key is pressed for a given key
	 * @param pin the pin on which the switch is attached
	 * @param callbackOnPressed the function to be called when a key is pressed
	 */
	void replaceOnPressed(pinid_t pin, KeyCallbackFn callbackOnPressed);

	/**
	 * Allows for a change of the listener that reports when a key is pressed for a given key
	 * @param pin the pin on which the switch is attached
	 * @param newListener the new object to receive updates when a key is pressed
	 */
	void replaceSwitchListener(pinid_t pin, SwitchListener* newListener);

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
	 * maximum value that can be represented and also the current value of the encoder. This always affects
	 * slot 0; which is normally the default rotary encoder.
	 * @param precision the maximum value to be set
	 * @param currentValue the current value to be set.
	 */
	void changeEncoderPrecision(uint16_t precision, uint16_t currentValue) { changeEncoderPrecision(0, precision, currentValue, false, 1); }

    /**
     * This is helper function that calls the rotary encoders change precision function. It changes the
     * maximum value that can be represented and also the current value of the encoder. The encoder to change
     * is selected by the slot paramter.
	 * @param slot the index of the desired encoder, zero based
     * @param precision the maximum value to be set
     * @param currentValue the current value to be set.
     */
    void changeEncoderPrecision(uint8_t slot, uint16_t precision, uint16_t currentValue) { changeEncoderPrecision(slot, precision, currentValue, false, 1); }

    /**
	 * Use this version of changeEncoderPrecision if you are working with more than one rotary encoder.
	 * This is helper function that calls the rotary encoders change precision function. It changes the
	 * maximum value that can be represented and also the current value of the encoder.
	 * @param slot the index of the desired encoder, zero based
	 * @param precision the maximum value to be set
	 * @param currentValue the current value to be set.
	 * @param rollover if the encoder should wrap around at min/max values or stop
 	 * @param step the size of each step of the encoder, default is 1
	 */
	void changeEncoderPrecision(uint8_t slot, uint16_t precision, uint16_t currentValue, bool rollover, int step=1);

	/**
	 * Simulates a switch press by calling the callback directly without changing the internal state
	 * of the key. Useful to simulate a key press in some situations.
	 * @param pin the pin associated with the switch
	 * @param held if the held state should be set on the callback
	 */
	void pushSwitch(pinid_t pin, bool held);

	/**
	 * This will normally be called by task manager when not interrupt driven.
	 */
	bool runLoop();

	/** Gets the IoAbstraction that is being used */
	IoAbstractionRef getIoAbstraction() { return ioDevice; }

	/**
	 * Returns true if the logic should be pull up, otherwise false.
     * @param invertedLogic indicates if the key we are checking for is invertedLogic
     * @return true if pull up style switching, otherwise false.
	 */
	bool isPullupLogic(bool invertedLogic) {
        bool pullUp = bitRead(swFlags, SW_FLAG_PULLUP_LOGIC);
    	// check if we need to invert the state, basically when the two states don't match.
        return (pullUp && !invertedLogic) || (!pullUp && invertedLogic);
    }
	
    /** @return if encoders are configured as polling or interrupt only */
	bool isEncoderPollingEnabled() {return bitRead(swFlags, SW_FLAG_ENCODER_IS_POLLING);}

	bool isInterruptDriven() {return bitRead(swFlags, SW_FLAG_INTERRUPT_DRIVEN);}

    /** @return if interrupt debouncing is in progress */
	bool isInterruptDebouncing() {return bitRead(swFlags, SW_FLAG_INTERRUPT_DEBOUNCE);}

	/**
	 * Returns true if the switch at the defined pin is pressed, otherwise false
	 * @param pin the pin to check if pressed
	 */
	bool isSwitchPressed(pinid_t pin);

	/** 
	 * Sets the debounce state - only really for internal use.
	 * @param debounce true if debouncing.
	 */
	void setInterruptDebouncing(bool debounce) { bitWrite(swFlags, SW_FLAG_INTERRUPT_DEBOUNCE, debounce);}

    /**
     * Gets the last sync status of the IoAbstraction being used by switches.
     * @return the last sync status as an bool, true for success, otherwise false.
     */
    bool didLastSyncSucceed() { return lastSyncStatus; }

    /**
     * Removes all switch definitions and resets switches as if it were just initialised.
     * IMPORTANT: no attempt is made to deregister any resources or remove registered interrupts.
     */
    void resetAllSwitches();

    /**
     * Remove a switch by it's pin reference. Returns true if able to remove the pin
     * IMPORTANT: no attempt is made to deregister any resources or remove registered interrupts.
     * @param pin the pin number to remove
     * @return true if removed, otherwise false.
     */
    bool removeSwitch(pinid_t pin) {
        return keys.removeByKey(pin);
    }

private:
    bool internalAddSwitch(pinid_t pin, bool invertLogic);

	friend void onSwitchesInterrupt(pinid_t);
};

/**
 * This is the global switch input variable. Do not create other instances of this class.
 */
extern SwitchInput switches;

/**
 * Initialise a hardware rotary encoder on the pins passed in, when the value changes the callback function
 * will be called. This library will set pinA and pinB to INPUT_PULLUP, and debounces internally. In most
 * cases no additional components are needed. This function automatically adds the encoder to the global
 * switches instance. This uses the state machine encoder, which is presently in BETA at best.
 *
 * @param pinA the first pin of the encoder, this pin must handle interrupts.
 * @param pinB the third pin of the encoder, the middle pin goes to ground.
 * @param callback the function that will receive the new state of the encoder on changes.
 * @param accelerationMode the mode of acceleration to use
 * @param encoderType the type of encoder being used
 */
void setupStateMachineRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderCallbackFn callback, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType encoderType = FULL_CYCLE);

/**
 * Initialise a hardware rotary encoder on the pins passed in, when the value changes the OO listener
 * will be invoked. This library will set pinA and pinB to INPUT_PULLUP, and debounces internally. In most
 * cases no additional components are needed. This function automatically adds the encoder to the global
 * switches instance. This uses the state machine encoder, which is presently in BETA at best.
 *
 * Essentially this does:
 *
 * 	    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, callback);
 *	    switches.setEncoder(0, enc);
 *
 * @param pinA the first pin of the encoder, this pin must handle interrupts.
 * @param pinB the third pin of the encoder, the middle pin goes to ground.
 * @param listener the function that will receive the new state of the encoder on changes.
 * @param accelerationMode the mode of acceleration to use
 * @param encoderType the type of encoder being used
 */
void setupStateMachineRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderListener* listener, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType encoderType = FULL_CYCLE);


/**
 * Initialise a hardware rotary encoder on the pins passed in, when the value changes the callback function
 * will be called. This library will set pinA and pinB to INPUT_PULLUP, and debounces internally. In most
 * cases no additional components are needed. This function automatically adds the encoder to the global
 * switches instance.
 *
 * Essentially this does:
 *
 * 	    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, callback);
 *	    switches.setEncoder(0, enc);
 *
 * @param pinA the first pin of the encoder, this pin must handle interrupts.
 * @param pinB the third pin of the encoder, the middle pin goes to ground.
 * @param callback the function that will receive the new state of the encoder on changes.
 * @param accelerationMode the mode of acceleration to use
 * @param encoderType the type of encoder being used
 */
void setupRotaryEncoderWithInterrupt(pinid_t pinA, pinid_t pinB, EncoderCallbackFn callback, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType encoderType = FULL_CYCLE);

/**
 * Initialise a hardware rotary encoder on the pins passed in, when the value changes the OO listener
 * will be invoked. This library will set pinA and pinB to INPUT_PULLUP, and debounces internally. In most
 * cases no additional components are needed. This function automatically adds the encoder to the global
 * switches instance.
 *
 * Essentially this does:
 *
 * 	    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, callback);
 *	    switches.setEncoder(0, enc);
 *
 * @param pinA the first pin of the encoder, this pin must handle interrupts.
 * @param pinB the third pin of the encoder, the middle pin goes to ground.
 * @param listener the function that will receive the new state of the encoder on changes.
 * @param accelerationMode the mode of acceleration to use
 * @param encoderType the type of encoder being used
 */
void setupRotaryEncoderWithInterrupt(pinid_t pinA, pinid_t pinB, EncoderListener* listener, HWAccelerationMode accelerationMode = HWACCEL_REGULAR, EncoderType encoderType = FULL_CYCLE);

/**
 * Initialise an encoder that uses up and down buttons to handle the same functions as a hardware encoder.
 * This function automatically adds the encoder to the global switches instance.
 *
 * Essentially this does:
 *
 * 	    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, callback);
 *	    switches.setEncoder(0, enc);
 *
 * @param pinUp the up button
 * @param pinDown the down button
 * @param callback the function that will receive the new state on change.
 */
void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, EncoderCallbackFn callback, int speed = 20);

/**
 * Initialise an encoder that uses up and down buttons to handle the same functions as a hardware encoder.
 * This function automatically adds the encoder to the global switches instance. This version takes an OO
 * listener implementing EncoderListener.
 *
 * Essentially this does:
 *
 * 	    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, listener);
 *	    switches.setEncoder(0, enc);
 *
 * @param pinUp the up button
 * @param pinDown the down button
 * @param listener the OO listener that will receive the new state on change.
 */
void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, EncoderListener* listener, int speed = 20);

/**
 * Initialise an encoder that uses up and down buttons to handle the same functions as a hardware encoder.
 * This function automatically adds the encoder to the global switches instance. This version takes an OO
 * listener implementing EncoderListener. This version supports the idea of rotating the cursor key operation
 * if needed by changing the intent to be sideways.
 *
 * @param pinUp the up button
 * @param pinDown the down button
 * @param pinLeft the left button
 * @param pinRight the right button
 * @param passThroughListener a listener that will receive notifications for left and right presses.
 * @param listener the OO listener that will receive the new state on change.
 */
void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, pinid_t pinLeft, pinid_t pinRight, SwitchListener* passThroughListener, EncoderListener* listener, int speed=20);

    /**
 * Initialise an encoder that uses up and down buttons to handle the same functions as a hardware encoder.
 * This function automatically adds the encoder to the global switches instance. This version takes an OO
 * listener implementing EncoderListener. This version supports the idea of rotating the cursor key operation
 * if needed by changing the intent to be sideways.
 *
 * @param pinUp the up button
 * @param pinDown the down button
 * @param pinLeft the left button
 * @param pinRight the right button
 * @param passThroughListener a listener that will receive notifications for left and right presses.
 * @param encoderCallbackFn will receive the new value on change.
 */
 void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, pinid_t pinLeft, pinid_t pinRight, SwitchListener* passThroughListener, EncoderCallbackFn encoderCallbackFn, int speed=20);

#endif
