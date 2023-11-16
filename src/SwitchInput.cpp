/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <inttypes.h>
#include "SwitchInput.h"

#define ONE_TURN_OF_ENCODER 32

SwitchInput switches;

void registerInterrupt(pinid_t pin);
void onSwitchesInterrupt(__attribute__((unused)) pinid_t pin);

KeyboardItem::KeyboardItem() : stateFlags(NOT_PRESSED), previousState(NOT_PRESSED), pin(-1), counter(0), acceleration(0),
                               repeatInterval(NO_REPEAT), notify{}, callbackOnRelease{} {}

KeyboardItem::KeyboardItem(pinid_t pin, KeyCallbackFn callback, uint8_t repeatInterval, bool keyLogicIsInverted) : notify{} {
    this->repeatInterval = repeatInterval;
    this->pin = pin;
	this->counter = 0;
    this->notify.callback = callback;
	previousState = NOT_PRESSED;
	stateFlags = NOT_PRESSED;
	callbackOnRelease = nullptr;
	acceleration = 0;
	bitWrite(stateFlags, KEY_LISTENER_MODE_BIT, 0);
	bitWrite(stateFlags, KEY_LOGIC_IS_INVERTED, keyLogicIsInverted);
}

KeyboardItem::KeyboardItem(pinid_t pin, SwitchListener* switchListener, uint8_t repeatInterval, bool keyLogicIsInverted) : notify{} {
	this->pin = pin;
	this->repeatInterval = repeatInterval;
    this->notify.listener = switchListener;
    this->counter = 0;
    previousState = NOT_PRESSED;
	stateFlags = NOT_PRESSED;
	callbackOnRelease = nullptr;
	acceleration = 0;
	bitWrite(stateFlags, KEY_LISTENER_MODE_BIT, 1);
	bitWrite(stateFlags, KEY_LOGIC_IS_INVERTED, keyLogicIsInverted);
}

KeyboardItem::KeyboardItem(const KeyboardItem& other) = default;
KeyboardItem& KeyboardItem::operator=(const KeyboardItem& other) = default;

void KeyboardItem::onRelease(KeyCallbackFn cb) {
	this->callbackOnRelease = cb;
}

void KeyboardItem::trigger(bool held) {
	if (!notify.callback) return;

	if (isUsingListener()) notify.listener->onPressed(pin, held);
	else notify.callback(pin, held);
}

void KeyboardItem::triggerRelease(bool held) {
	if (isUsingListener()) notify.listener->onReleased(pin, held);
	else if(callbackOnRelease) callbackOnRelease(pin, held);
}

void KeyboardItem::changeOnPressed(KeyCallbackFn pFunction) {
    bitWrite(stateFlags, KEY_LISTENER_MODE_BIT, 0);
    this->notify.callback = pFunction;
}

void KeyboardItem::changeListener(SwitchListener* listener) {
    bitWrite(stateFlags, KEY_LISTENER_MODE_BIT, 1);
    this->notify.listener = listener;
}


void KeyboardItem::checkAndTrigger(uint8_t buttonState){
	if (notify.callback == nullptr && callbackOnRelease == nullptr) return;

	if (buttonState == HIGH) {
		if (getState() == NOT_PRESSED) {
			setState(DEBOUNCING1);
		}
		else if (isDebouncing()) {
			setState(PRESSED);
			previousState = PRESSED;
			counter = 0; 
			acceleration = 1;
			trigger(false);
				
		}
		else if (getState() == PRESSED) {
			counter++;
			if (counter > HOLD_THRESHOLD) {
				setState(BUTTON_HELD);
				previousState = BUTTON_HELD;
				trigger(true);
				counter = 0;
				acceleration = 1;
			}
		}
		else if (getState() == BUTTON_HELD && repeatInterval != NO_REPEAT && notify.callback != nullptr) {
			counter = counter + (acceleration >> SWITCHES_ACCELERATION_DIVISOR) + 1;
			if (counter > repeatInterval) {
				acceleration = min(255, acceleration + 1);
				trigger(true);
				counter = 0;
			}
		}
	}
	else if(getState() == DEBOUNCING1) {
		setState(DEBOUNCING2);
	}
	else {
		setState(NOT_PRESSED);
		if (previousState == PRESSED) {
			previousState = NOT_PRESSED;
			triggerRelease(false);
		} else if (previousState == BUTTON_HELD){
			previousState = NOT_PRESSED;
			triggerRelease(true);
		}
	}
}

SwitchInput::SwitchInput() : encoder{}, keys(MAX_KEYS) {
	this->ioDevice = nullptr;
	this->swFlags = 0;
    this->lastSyncStatus = true;
}

void SwitchInput::initialiseInterrupt(IoAbstractionRef device, bool usePullUpSwitching) {
	this->init(device, SWITCHES_NO_POLLING, usePullUpSwitching);
}

void SwitchInput::initialise(IoAbstractionRef device, bool usePullUpSwitching) {
	this->init(device, SWITCHES_POLL_KEYS_ONLY, usePullUpSwitching);
}

void SwitchInput::init(IoAbstractionRef device, SwitchInterruptMode mode, bool defaultIsPullUp) {
	this->ioDevice = device;

	// set up the flags
	this->swFlags = 0;
	bitWrite(swFlags, SW_FLAG_PULLUP_LOGIC, defaultIsPullUp);
	bitWrite(swFlags, SW_FLAG_INTERRUPT_DRIVEN, (mode == SWITCHES_NO_POLLING));
	bitWrite(swFlags, SW_FLAG_ENCODER_IS_POLLING, (mode == SWITCHES_POLL_EVERYTHING));

	if(mode == SWITCHES_POLL_KEYS_ONLY) {
		serlogF(SER_IOA_INFO, "Switches polling for keys");
		taskManager.scheduleFixedRate(SWITCH_POLL_INTERVAL, [] {
			switches.runLoop();
		});
	} else if(mode == SWITCHES_POLL_EVERYTHING) {
        serlogF(SER_IOA_INFO, "Switches polling for everything");
		taskManager.scheduleFixedRate(SWITCH_POLL_INTERVAL / 8, [] {
            static uint8_t counter = 0;
            if(++counter % 8 == 7) {
                switches.runLoop();
            }
			onSwitchesInterrupt(-1);
		});
	}

	serlogF4(SER_IOA_INFO, "Switches initialized (pull-up, int, encPoll)", bitRead(swFlags, SW_FLAG_PULLUP_LOGIC), bitRead(swFlags, SW_FLAG_INTERRUPT_DRIVEN),
			   bitRead(swFlags, SW_FLAG_ENCODER_IS_POLLING));
}

bool SwitchInput::addSwitch(pinid_t pin, KeyCallbackFn callback,uint8_t repeat, bool invertLogic) {
	internalAddSwitch(pin, invertLogic);
    return keys.add(KeyboardItem(pin, callback, repeat, invertLogic));
}

bool SwitchInput::addSwitchListener(pinid_t pin, SwitchListener* listener, uint8_t repeat, bool invertLogic) {
	internalAddSwitch(pin, invertLogic);
    return keys.add(KeyboardItem(pin, listener, repeat, invertLogic));
}

bool SwitchInput::internalAddSwitch(pinid_t pin, bool invertLogic) {
	if (ioDevice == nullptr) initialise(internalDigitalIo(), true);

	ioDevice->pinMode(pin, isPullupLogic(invertLogic) ? INPUT_PULLUP : INPUT);

    if (isInterruptDriven()) {
		registerInterrupt(pin);
	}

    return true;
}

void SwitchInput::onRelease(pinid_t pin, KeyCallbackFn callbackOnRelease) {
	if (ioDevice == nullptr) initialise(internalDigitalIo(), true);

	auto keyItem = keys.getByKey(pin);
	if(keyItem) {
	    // already initialised, just add the release callback
	    keyItem->onRelease(callbackOnRelease);
	}
	else {
        internalAddSwitch(pin, false);
	    // not yet added, we will do a best efforts standard initialisation.
        KeyboardItem newItem(pin, (KeyCallbackFn) nullptr, NO_REPEAT, false);
        newItem.onRelease(callbackOnRelease);
        keys.add(newItem);
    }
}

void SwitchInput::replaceOnPressed(pinid_t pin, KeyCallbackFn callbackOnPressed) {
    auto keyItem = keys.getByKey(pin);
    if(keyItem) {
        keyItem->changeOnPressed(callbackOnPressed);
    }
}

void SwitchInput::replaceSwitchListener(pinid_t pin, SwitchListener* newListener) {
    auto keyItem = keys.getByKey(pin);
    if(keyItem) {
        keyItem->changeListener(newListener);
    }
}

bool SwitchInput::isSwitchPressed(pinid_t pin) {
    return keys.getByKey(pin)->isPressed();
}

void SwitchInput::pushSwitch(pinid_t pin, bool held) {
    keys.getByKey(pin)->trigger(held);
}

void SwitchInput::changeEncoderPrecision(uint8_t slot, uint16_t precision, uint16_t currentValue, bool rollover, int step) {
	if (slot < MAX_ROTARY_ENCODERS && encoder[slot] != nullptr) {
		encoder[slot]->changePrecision(precision, (int)currentValue, rollover, step);
	}
}

void SwitchInput::setEncoder(uint8_t slot, RotaryEncoder* enc) {
	if (slot < MAX_ROTARY_ENCODERS) {
		this->encoder[slot] = enc;
	}
}

bool SwitchInput::runLoop() {
	bool needAnotherGo = false;

	lastSyncStatus = ioDevice->sync();

	for (bsize_t i = 0; i < keys.count(); ++i) {
		// get the pins current state
		auto key = keys.itemAtIndex(i);
		uint8_t pinState = ioDevice->digitalRead(key->getPin());
		if(isPullupLogic(key->isLogicInverted())) {
			pinState = !pinState;
		}
		// and pass to the key handler.
		key->checkAndTrigger(pinState);

		// we need to call into here again if we are debouncing or anything is pressed.
		needAnotherGo |= (key->isDebouncing() || key->isPressed());
	}

	return needAnotherGo;
}


/******ROTARY ENCODERS *****/


RotaryEncoder::RotaryEncoder(EncoderCallbackFn callback) : notify{} {
    this->notify.callback = callback;
	this->currentReading = 0;
	this->maximumValue = 0;
    this->flags = 0U;
    bitWrite(flags, LAST_SYNC_STATUS, 1);
    this->intent = CHANGE_VALUE;
}

RotaryEncoder::RotaryEncoder(EncoderListener* listener) : notify{} {
	this->notify.encoderListener = listener;
	this->currentReading = 0;
	this->maximumValue = 0;
    this->stepSize = 1;
    this->flags = 0U;
    bitWrite(flags, LAST_SYNC_STATUS, 1);
    bitWrite(flags, OO_LISTENER_CALLBACK, 1);
    this->intent = CHANGE_VALUE;
}

void RotaryEncoder::changePrecision(uint16_t maxValue, int currentValue, bool rolloverOnMax, int step) {
	this->maximumValue = maxValue;
	this->currentReading = currentValue;
    this->stepSize = step;
	bitWrite(flags, WRAP_AROUND_MODE, rolloverOnMax);
	intent = (maxValue == 0U && currentValue == 0) ? DIRECTION_ONLY : CHANGE_VALUE;
	runCallback((int)currentReading);
}

void RotaryEncoder::replaceCallback(EncoderCallbackFn callbackFn) {
    bitWrite(flags, OO_LISTENER_CALLBACK, false);
    this->notify.callback = callbackFn;
}

void RotaryEncoder::replaceCallbackListener(EncoderListener* listener) {
    bitWrite(flags, OO_LISTENER_CALLBACK, true);
    this->notify.encoderListener = listener;
}

void RotaryEncoder::setUserIntention(EncoderUserIntention intention) {
    intent = intention;
    if(intention == DIRECTION_ONLY) {
        maximumValue = 0;
        currentReading = 0;
    }
}

// this abs accounts for some boards where abs is a double precision function
#define safeAbs(x) ((x) < 0 ? -(x) : (x))

void RotaryEncoder::increment(int8_t incVal) {
    if(maximumValue == 0 && intent == DIRECTION_ONLY) {
		// first check if we are in direction only mode (max = 0)
		 runCallback(incVal);
         return;
	}

    bool rollover = bitRead(flags, WRAP_AROUND_MODE) != 0;
    if(incVal >= 0) {
        if(rollover) {
			currentReading = (currentReading + incVal);
			if (currentReading > maximumValue) currentReading = currentReading - maximumValue - 1;
        } else {
			currentReading = min((uint16_t)(currentReading + incVal), maximumValue);
		}
	} else if(currentReading < abs(incVal)) {
		currentReading = rollover? maximumValue - safeAbs(incVal) + 1 : 0;
	} else if(currentReading != 0) {
		currentReading += incVal;
    }
    runCallback((int)currentReading);
}

HardwareRotaryEncoder::HardwareRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderCallbackFn callback, HWAccelerationMode accelerationMode,
                                             EncoderType encoderType) : AbstractHwRotaryEncoder(callback) {
    initialise(pinA, pinB, accelerationMode, encoderType);
}

HardwareRotaryEncoder::HardwareRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderListener* listener, HWAccelerationMode accelerationMode,
                                             EncoderType encoderType) : AbstractHwRotaryEncoder(listener) {
    initialise(pinA, pinB, accelerationMode, encoderType);
}

void AbstractHwRotaryEncoder::initialiseBase(pinid_t pinA, pinid_t pinB, HWAccelerationMode accelerationMode, EncoderType encoderType) {
    this->pinA = pinA;
	this->pinB = pinB;
	this->lastChange = micros();
    this->accelerationMode = accelerationMode;
	this->encoderType = encoderType;

	// set the pin directions to input with pull ups enabled
	switches.getIoAbstraction()->pinMode(pinA, INPUT_PULLUP);
	switches.getIoAbstraction()->pinMode(pinB, INPUT_PULLUP);

	// read back the initial values.
    bool lastSyncOK = switches.getIoAbstraction()->sync();
	bitWrite(flags, LAST_SYNC_STATUS, lastSyncOK);

	if(!switches.isEncoderPollingEnabled()) {
		registerInterrupt(pinA);
		registerInterrupt(pinB);
	}
}

void checkRunLoopAndRepeat() {
	// turn off interrupts until debouncing / repeat logic is complete.
	switches.setInterruptDebouncing(true);

	// instead of running constantly, we only run when there's a need to, eg something
	// is still in a debouncing state. Otherwise we wait for an interrupt.
	// switches.runLoop returns true when it needs to run again.
	if(switches.runLoop() && switches.isInterruptDriven()) {
		taskManager.scheduleOnce(20, [] {
			checkRunLoopAndRepeat();
		});
	}
	else {
		// back to normal now - interrupt only
		switches.setInterruptDebouncing(false);
	}
}

void onSwitchesInterrupt(__attribute__((unused)) pinid_t pin) {
	if(switches.isInterruptDriven() && !switches.isInterruptDebouncing()) {
		checkRunLoopAndRepeat();
	}

  for(int i = 0; i < MAX_ROTARY_ENCODERS; ++i) {
		if(switches.encoder[i]) {
			switches.encoder[i]->encoderChanged();
		}
	}
}

void SwitchInput::resetAllSwitches() {
    keys.clear();
    ioDevice = internalDigitalIo();
    for(int i=0;i<MAX_ROTARY_ENCODERS;i++) {
        encoder[i] = nullptr;
    }
}

int AbstractHwRotaryEncoder::amountFromChange(unsigned long change) {
	if(change > 250000 || maximumValue < ONE_TURN_OF_ENCODER) return stepSize;

    if(accelerationMode == HWACCEL_NONE) {
        return stepSize;
    }
    else if(accelerationMode == HWACCEL_REGULAR) {
        if(change > 120000) return stepSize + stepSize;
        else if (change > 70000) return stepSize << 2;
        else if (change > 30000) return stepSize << 3;
        else return stepSize << 4;
    }
    else { // slower, very slight acceleration..
        if(change > 100000) return stepSize + stepSize;
        else if (change > 30000) return stepSize + stepSize + stepSize;
        else return stepSize << 2;
    }

}

void HardwareRotaryEncoder::encoderChanged() {
	bool lastSyncStatus = switches.getIoAbstraction()->sync();
    bitWrite(flags, LAST_SYNC_STATUS, lastSyncStatus);

	uint8_t a = switches.getIoAbstraction()->digitalRead(pinA);
	uint8_t b = switches.getIoAbstraction()->digitalRead(pinB);

	if(encoderType == QUARTER_CYCLE){
		if((a != aLast) || (b != cleanFromB)) {
			aLast = a;
			if((a != aLast) || (b != cleanFromB)) {
				cleanFromB = b;
				if((a || cleanFromB) || (a == 0 && b == 0)) {
                    handleChangeRaw(a && b);
                }
			}
		}		
	}
	else {
		if(a != aLast) {
			aLast = a;
			if(b != cleanFromB) {
				cleanFromB = b;
				if(a) {
                    handleChangeRaw(b);
				}
			}
		}	
	}
}

void HardwareRotaryEncoder::initialise(pinid_t pinA, pinid_t pinB, HWAccelerationMode accelerationMode, EncoderType et) {
    this->aLast = switches.getIoAbstraction()->digitalRead(pinA);
    this->cleanFromB = switches.getIoAbstraction()->digitalRead(pinB);
    initialiseBase(pinA, pinB, accelerationMode, et);
}

void AbstractHwRotaryEncoder::handleChangeRaw(bool increase) {
    // was the last direction up?
    bool lastDirectionUp = bitRead(flags, LAST_ENCODER_DIRECTION_UP);

    // get the amount of change and direction. Also keep a copy of the time until later for acceleration purposes
    unsigned long timeNow = micros();
    unsigned long deltaMillis = timeNow - lastChange;
    int amt = amountFromChange(deltaMillis);

    // update the last change time now to ensure always set
    lastChange = timeNow;

    // direction changes within the reject change threshold would not result in an encoder change, part of the debounce
    // logic to prevent spurious updates. Within this period direction must be the both now and previously.
    //serlogF4(SER_DEBUG, "delta ", deltaMillis, increase, lastDirectionUp);
    if(deltaMillis < REJECT_DIRECTION_CHANGE_THRESHOLD && increase != lastDirectionUp) return;

    // now we make the change and register the last change direction (as we accepted it)
    increment((int8_t) (increase ? amt : -amt));
    bitWrite(flags, LAST_ENCODER_DIRECTION_UP, increase);
}

EncoderUpDownButtons::EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, EncoderCallbackFn callback, uint8_t speed)
        : RotaryEncoder(callback), upPin(pinUp), downPin(pinDown), backPin(-1), nextPin(-1), passThroughListener(nullptr),
          canRotate(false) {
	switches.addSwitchListener(pinUp, this, speed);
	switches.addSwitchListener(pinDown, this, speed);
}

EncoderUpDownButtons::EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, EncoderListener* listener, uint8_t speed)
        : RotaryEncoder(listener), upPin(pinUp), downPin(pinDown), backPin(-1), nextPin(-1), passThroughListener(nullptr),
          canRotate(false){
    switches.addSwitchListener(pinUp, this, speed);
    switches.addSwitchListener(pinDown, this, speed);
}

EncoderUpDownButtons::EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, pinid_t backPin, pinid_t nextPin,
                                           SwitchListener* passThrough, EncoderCallbackFn callback, uint8_t speed)
        : RotaryEncoder(callback), upPin(pinUp), downPin(pinDown), backPin(backPin), nextPin(nextPin), passThroughListener(passThrough),
          canRotate(true) {
    switches.addSwitchListener(pinUp, this, speed);
    switches.addSwitchListener(pinDown, this, speed);
    switches.addSwitchListener(backPin, this, speed);
    switches.addSwitchListener(nextPin, this, speed);
}

EncoderUpDownButtons::EncoderUpDownButtons(pinid_t pinUp, pinid_t pinDown, pinid_t backPin, pinid_t nextPin,
                                           SwitchListener* passThrough, EncoderListener* listener, uint8_t speed)
        : RotaryEncoder(listener), upPin(pinUp), downPin(pinDown), backPin(backPin), nextPin(nextPin), passThroughListener(passThrough),
          canRotate(true) {
    switches.addSwitchListener(pinUp, this, speed);
    switches.addSwitchListener(pinDown, this, speed);
    switches.addSwitchListener(backPin, this, speed);
    switches.addSwitchListener(nextPin, this, speed);
}

void EncoderUpDownButtons::onPressed(pinid_t pin, bool held) {
    bool invert = intent == SCROLL_THROUGH_ITEMS;
    if (pin == getIncrementPin()) {
        int8_t dir = invert ? -stepSize : stepSize;
        increment(dir);
    } else if (pin == getDecrementPin()) {
        int8_t dir = invert ? stepSize : -stepSize;
        increment(dir);
    } else if(backPin != -1 && passThroughListener && pin == getBackPin()) {
        passThroughListener->onPressed(backPin, held);
    } else if(nextPin != -1 && passThroughListener && pin == getNextPin()) {
        passThroughListener->onPressed(nextPin, held);
    }
}

void EncoderUpDownButtons::onReleased(pinid_t pin, bool held) {
    if(backPin != -1 && passThroughListener && pin == getBackPin()) {
        passThroughListener->onReleased(backPin, held);
    } else if(nextPin != -1 && passThroughListener && pin == getNextPin()) {
        passThroughListener->onReleased(nextPin, held);
    }
}


enum EsWhenToOutput : uint8_t {
    OUTPUT_NEVER, OUTPUT_ALWAYS, OUTPUT_ONLY_QUARTER
};

struct EncoderState {
    uint8_t expectedBitPattern;
    EsWhenToOutput whenToOutput;
};

#define MAX_ENCODER_STATES 4

const EncoderState bitPatternStateMachine[] PROGMEM = {
        { 0b00, OUTPUT_ALWAYS },
        { 0b10, OUTPUT_NEVER },
        { 0b11, OUTPUT_ONLY_QUARTER },
        { 0b01, OUTPUT_NEVER },
};

#define bitPatternFrom(a, b) ( ((a) << 1) | (b) )

inline int8_t stateLimit(int x) {
    // never allow the array to be outside the range, it is effectively a circular buffer that keeps wrapping.
    if(x < 0) return MAX_ENCODER_STATES - 1;
    if(x >= MAX_ENCODER_STATES) return 0;
    return (int8_t)x;
}

int8_t HwStateRotaryEncoder::stateFor(uint8_t bits) {
    for(int i=0; i < MAX_ENCODER_STATES; i++) {
        if(bitPatternStateMachine[i].expectedBitPattern == bits) return i;
    }
    return 0;
}

void HwStateRotaryEncoder::encoderChanged() {
    bool lastSyncStatus = switches.getIoAbstraction()->sync();
    bitWrite(flags, LAST_SYNC_STATUS, lastSyncStatus);

    // get the current bit pattern on a and b
    uint8_t a = switches.getIoAbstraction()->digitalRead(pinA);
    uint8_t b = switches.getIoAbstraction()->digitalRead(pinB);

    if(currentEncoderState == -1) {
        if(a == 0 && b == 0) {
            currentEncoderState = 0;
        } else {
            return;
        }
    }

    // if the bit pattern is unchanged, do nothing.
    uint8_t bits = bitPatternFrom(a, b);
    const EncoderState& same =  bitPatternStateMachine[currentEncoderState];
    if(same.expectedBitPattern == bits) return; // unchanged.

    // check if we've gone up or down
    const EncoderState& prev =  bitPatternStateMachine[stateLimit(currentEncoderState + 1)];
    const EncoderState& next =  bitPatternStateMachine[stateLimit(currentEncoderState - 1)];
    int dir;
    if(prev.expectedBitPattern == bits) {
        currentEncoderState = stateLimit(currentEncoderState + 1);
        dir = false; // going down
    } else if(next.expectedBitPattern == bits) {
        currentEncoderState = stateLimit(currentEncoderState - 1);
        dir = true; // going up
    } else {
        currentEncoderState = -1; // mark invalid, do not output anything
        return;
    }

    // output the state change if needed
    auto whenToOutput = bitPatternStateMachine[currentEncoderState].whenToOutput;
    if(whenToOutput == OUTPUT_ALWAYS || (whenToOutput == OUTPUT_ONLY_QUARTER && encoderType != FULL_CYCLE)) {
        handleChangeRaw(dir);
    }
}

HwStateRotaryEncoder::HwStateRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderCallbackFn callback, HWAccelerationMode accelerationMode,
                                           EncoderType encoderType) : AbstractHwRotaryEncoder(callback) {
    initialiseBase(pinA, pinB, accelerationMode, encoderType);
}

HwStateRotaryEncoder::HwStateRotaryEncoder(pinid_t pinA, pinid_t pinB, EncoderListener* listener, HWAccelerationMode accelerationMode,
                                           EncoderType encoderType) : AbstractHwRotaryEncoder(listener) {
    initialiseBase(pinA, pinB, accelerationMode, encoderType);
}


/******** ENCODER SETUP METHODS ***********/

void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, EncoderCallbackFn callback, int speed) {
	if (switches.getIoAbstraction() == nullptr) switches.init(internalDigitalIo(), SWITCHES_POLL_EVERYTHING, true);

	auto* enc = new EncoderUpDownButtons(pinUp, pinDown, callback, speed);
	switches.setEncoder(enc);
}

void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, EncoderListener* listener, int speed) {
    if (switches.getIoAbstraction() == nullptr) switches.init(internalDigitalIo(), SWITCHES_POLL_EVERYTHING, true);

    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, listener, speed);
    switches.setEncoder(enc);
}

void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, pinid_t pinLeft, pinid_t pinRight, SwitchListener* passThroughListener, EncoderListener* listener, int speed) {
    if (switches.getIoAbstraction() == nullptr) switches.init(internalDigitalIo(), SWITCHES_POLL_EVERYTHING, true);

    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, pinLeft, pinRight, passThroughListener, listener, speed);
    switches.setEncoder(enc);
}
void setupUpDownButtonEncoder(pinid_t pinUp, pinid_t pinDown, pinid_t pinLeft, pinid_t pinRight, SwitchListener* passThroughListener, EncoderCallbackFn cb, int speed) {
    if (switches.getIoAbstraction() == nullptr) switches.init(internalDigitalIo(), SWITCHES_POLL_EVERYTHING, true);

    auto* enc = new EncoderUpDownButtons(pinUp, pinDown, pinLeft, pinRight, passThroughListener, cb, speed);
    switches.setEncoder(enc);
}

void registerInterrupt(pinid_t pin) {
	taskManager.setInterruptCallback(onSwitchesInterrupt);
	taskManager.addInterrupt(switches.getIoAbstraction(), pin, CHANGE);
}

void setupRotaryEncoderWithInterrupt(pinid_t pinA, pinid_t pinB, EncoderCallbackFn callback, HWAccelerationMode accelerationMode, EncoderType encoderType) {
	if (switches.getIoAbstraction() == nullptr) switches.init(internalDigitalIo(), SWITCHES_POLL_EVERYTHING, true);

#ifdef TC_LEGACY_ENCODER
	switches.setEncoder(new HardwareRotaryEncoder(pinA, pinB, callback, accelerationMode, encoderType));
#else
    switches.setEncoder(new HwStateRotaryEncoder(pinA, pinB, callback, accelerationMode, encoderType));
#endif
}

void setupRotaryEncoderWithInterrupt(pinid_t pinA, pinid_t pinB, EncoderListener* listener, HWAccelerationMode accelerationMode, EncoderType encoderType) {
	if (switches.getIoAbstraction() == nullptr) switches.init(internalDigitalIo(), SWITCHES_POLL_EVERYTHING, true);

#ifdef TC_LEGACY_ENCODER
	switches.setEncoder(new HardwareRotaryEncoder(pinA, pinB, listener, accelerationMode, encoderType));
#else
    switches.setEncoder(new HwStateRotaryEncoder(pinA, pinB, listener, accelerationMode, encoderType));
#endif
}
