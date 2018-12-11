/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <inttypes.h>
#include <Arduino.h>
#include "SwitchInput.h"

SwitchInput switches;

void registerInterrupt(uint8_t pin);

KeyboardItem::KeyboardItem() {
	this->repeatInterval = NO_REPEAT;
	this->pin = -1;
	this->callback = NULL;
	this->counter = 0;
	this->state = NOT_PRESSED;
	this->previousState = NOT_PRESSED;
	this->callbackOnRelease = NULL;
}

void KeyboardItem::initialise(uint8_t pin, KeyCallbackFn callback, uint8_t repeatInterval) {
	this->callback = callback;
	this->pin = pin;
	this->repeatInterval = repeatInterval;
	state = NOT_PRESSED;
	previousState = NOT_PRESSED;
}

void KeyboardItem::onRelease(KeyReleasedCallbackFn callbackOnRelease) {
	this->callbackOnRelease = callbackOnRelease;
}

void KeyboardItem::checkAndTrigger(uint8_t buttonState){
	if (callback == NULL && callbackOnRelease == NULL) return; 
	
	if (buttonState == HIGH) {
		if (state == NOT_PRESSED) {
			state = DEBOUNCING1;
		}
		else if (isDebouncing()) {
			state = PRESSED;
			if (callbackOnRelease != NULL) previousState = PRESSED;
			if (callback != NULL) {
				counter = 0; 
				(*callback)(pin, false);
			} 		
		}
		else if (state == PRESSED && callback != NULL) {
			if (++counter > HOLD_THRESHOLD) {
				state = BUTTON_HELD;
				(*callback)(pin, true);
				counter = 0;
			}
		}
		else if (state == BUTTON_HELD && repeatInterval != NO_REPEAT && callback != NULL) {
			if (++counter > repeatInterval) {
				(*callback)(pin, true);
				counter = 0;
			}
		}
	}
	else if(state == DEBOUNCING1) {
		state = DEBOUNCING2;
	}
	else {
		state = NOT_PRESSED;
		if (previousState == PRESSED) {
			previousState = NOT_PRESSED;
			(*callbackOnRelease)(pin);
		}
	}
}

SwitchInput::SwitchInput() {
	this->numberOfKeys = 0;	
	this->ioDevice = NULL;
	this->encoder = NULL;
	this->swFlags = 0;
}

void SwitchInput::initialiseInterrupt(IoAbstractionRef ioDevice, bool usePullUpSwitching) {
	this->ioDevice = ioDevice;
	this->swFlags = 0;
	this->numberOfKeys = 0;
	bitWrite(swFlags, SW_FLAG_PULLUP_LOGIC, usePullUpSwitching);
	bitSet(swFlags, SW_FLAG_INTERRUPT_DRIVEN);

	// do not start any tasks here, we need to register interrupt on the pins instead.
}

void SwitchInput::initialise(IoAbstractionRef ioDevice, bool usePullUpSwitching) {
	this->ioDevice = ioDevice;
	this->swFlags = 0;
	this->numberOfKeys = 0;
	bitWrite(swFlags, SW_FLAG_PULLUP_LOGIC, usePullUpSwitching);

	taskManager.scheduleFixedRate(20, [] {
		switches.runLoop();
	});

}

void SwitchInput::addSwitch(uint8_t pin, KeyCallbackFn callback,uint8_t repeat) {
	keys[numberOfKeys++].initialise(pin, callback, repeat);
	ioDevicePinMode(ioDevice, pin, isPullupLogic() ? INPUT_PULLUP : INPUT);

	if(isInterruptDriven()) {
		registerInterrupt(pin);
	}
}

void SwitchInput::onRelease(uint8_t pin, KeyReleasedCallbackFn callbackOnRelease) {
	for(uint8_t i=0; i<numberOfKeys; ++i) {
		if(keys[i].getPin() == pin) {
			keys[i].onRelease(callbackOnRelease);
			return;
		}
	}
}

void SwitchInput::pushSwitch(uint8_t pin, bool held) {
	for(uint8_t i=0; i<numberOfKeys; ++i) {
		if(keys[i].getPin() == pin) {
			keys[i].trigger(held);
			return;
		}
	}
}

void SwitchInput::changeEncoderPrecision(uint16_t precision, uint16_t currentValue) {
	if(encoder != NULL) {
		encoder->changePrecision(precision, currentValue);
	}
}

bool SwitchInput::runLoop() {
	bool needAnotherGo = false;

	ioDeviceSync(ioDevice);

	for (int i = 0; i < numberOfKeys; ++i) {
		// get the pins current state
		uint8_t pinState = ioDeviceDigitalRead(ioDevice, keys[i].getPin());
		// if the switches are pull up, invert the state.
		if(isPullupLogic()) {
			pinState = !pinState;
		}
		// and pass to the key handler.
		keys[i].checkAndTrigger(pinState);

		// we need to call into here again if we are debouncing or anything is pressed.
		needAnotherGo |= (keys[i].isDebouncing() || keys[i].isPressed());
	}

	return needAnotherGo;
}


/******ROTARY ENCODERS *****/


RotaryEncoder::RotaryEncoder(EncoderCallbackFn callback) {
	this->callback = callback;
	this->currentReading = 0;
	this->maximumValue = 0;
}

void RotaryEncoder::changePrecision(uint16_t maxValue, int currentValue) {
	this->maximumValue = maxValue;
	this->currentReading = currentValue;
	callback(currentReading);
}

void RotaryEncoder::increment(int8_t incVal) {
	if (((currentReading) == 0 && (incVal < 0)) || ((currentReading + incVal) > maximumValue)) return;
	currentReading += incVal; callback(currentReading);
}

HardwareRotaryEncoder::HardwareRotaryEncoder(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback) : RotaryEncoder(callback) {
	this->pinA = pinA;
	this->pinB = pinB;

	// set the pin directions to input with pull ups enabled
	ioDevicePinMode(switches.getIoAbstraction(), pinA, INPUT_PULLUP);
	ioDevicePinMode(switches.getIoAbstraction(), pinB, INPUT_PULLUP);

	// read back the initial values.
	ioDeviceSync(switches.getIoAbstraction());	
	this->aLast = ioDeviceDigitalRead(switches.getIoAbstraction(), pinA);
	this->cleanFromB = ioDeviceDigitalRead(switches.getIoAbstraction(), pinB);

	registerInterrupt(pinA);
}

void checkRunLoopAndRepeat() {
	// turn off interrupts until deboucing / repeat logic is complete.
	switches.setInterruptDebouncing(true);

	// instead of running constantly, we only run when there's a need to, eg something
	// is still in a debouncing state. Otherwise we wait for an interrupt.
	// switches.runLoop returns true when it needs to run again.
	if(switches.runLoop()) {
		taskManager.scheduleOnce(20, [] { 
			checkRunLoopAndRepeat();
		});
	}
	else {
		// back to normal now - interrupt only
		switches.setInterruptDebouncing(false);
	}
}

void onSwitchesInterrupt(__attribute__((unused)) uint8_t pin) {
	if(switches.isInterruptDriven() && !switches.isInterruptDebouncing()) {
		checkRunLoopAndRepeat();
	}

	if(switches.encoder) {
		switches.encoder->encoderChanged();
	}
}

void HardwareRotaryEncoder::encoderChanged() {
	ioDeviceSync(switches.getIoAbstraction());
	uint8_t a = ioDeviceDigitalRead(switches.getIoAbstraction(), pinA);
	uint8_t b = ioDeviceDigitalRead(switches.getIoAbstraction(), pinB);

	if(a != aLast) {
		aLast = a;
		if(b != cleanFromB) {
			cleanFromB = b;
			if(a) {	
				increment(a != b ? -1 : +1);
			}
		}
	}
}

/******** UP DOWN BUTTON ENCODER *******/

void switchEncoderUp(__attribute((unused)) uint8_t key, __attribute((unused)) bool heldDown) {
	switches.encoder->increment(1);
}

void switchEncoderDown(__attribute((unused)) uint8_t key, __attribute((unused)) bool heldDown) {
	switches.encoder->increment(-1);
}

EncoderUpDownButtons::EncoderUpDownButtons(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback, uint8_t speed) : RotaryEncoder(callback) {
	switches.addSwitch(pinUp, switchEncoderUp, speed);
	switches.addSwitch(pinDown, switchEncoderDown, speed);
}

/******** ENCODER SETUP METHODS ***********/

void setupUpDownButtonEncoder(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback) {
	EncoderUpDownButtons* enc = new EncoderUpDownButtons(pinUp, pinDown, callback);
	switches.setEncoder(enc);
}

void registerInterrupt(uint8_t pin) {
	taskManager.setInterruptCallback(onSwitchesInterrupt);
	taskManager.addInterrupt(switches.getIoAbstraction(), pin, CHANGE);
}

void setupRotaryEncoderWithInterrupt(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback) {
	switches.setEncoder(new HardwareRotaryEncoder(pinA, pinB, callback));
}
