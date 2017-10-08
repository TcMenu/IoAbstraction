#include <inttypes.h>
#include <Arduino.h>
#include "SwitchInput.h"

KeyboardItem::KeyboardItem() {
	this->repeatInterval = NO_REPEAT;
	this->pin = -1;
}

void KeyboardItem::initialise(uint8_t pin, uint8_t repeatInterval = NO_REPEAT) {
	this->pin = pin;
	this->repeatInterval = repeatInterval;
	state = NOT_PRESSED;
}

void KeyboardItem::checkAndTrigger(uint8_t buttonState, KeyCallbackFn callback){
	if (callback == NULL) return;
	
	if (buttonState == HIGH) {
		if (state == NOT_PRESSED) {
			state = DEBOUNCING1;
		}
		else if (isDebouncing()) {
			state = PRESSED;
			counter = 0;
			(*callback)(pin, false);
		}
		else if (state == PRESSED) {
			if (++counter > HOLD_THRESHOLD) {
				state = BUTTON_HELD;
				(*callback)(pin, true);
			}
		}
		else if (state == BUTTON_HELD && repeatInterval != NO_REPEAT) {
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
	}
}

RotaryEncoder::RotaryEncoder() {
	pinA = 0xff;
	pinB = 0xff;
	precision = 8;
	currentReading = 0;
	lastStates = 0;
}

void RotaryEncoder::initialise(uint8_t pinA, uint8_t pinB, uint8_t precision) {
	this->pinA = pinA;
	this->pinB = pinB;

	this->precision = precision;
	this->currentReading = 0;
	this->lastStates = 0;
}


SwitchInput::SwitchInput(BasicIoAbstraction* ioDevice) {
	this->numberOfKeys = 0;
	this->ioDevice = ioDevice;
}

void SwitchInput::addSwitch(uint8_t pin, uint8_t repeat = NO_REPEAT) {
	keys[numberOfKeys++].initialise(pin, repeat);
	ioDevice->pinDirection(pin, INPUT);
}

void SwitchInput::addRotataryEncoder(uint8_t pinA, uint8_t pinB, uint8_t precision) {

	ioDevicePinMode(ioDevice, pinA, INPUT);
	ioDevicePinMode(ioDevice, pinB, INPUT);

	encoder.initialise(pinA, pinB, precision);
}

void SwitchInput::runLoop() {
	ioDevice->runLoop();
	for (int i = 0; i < numberOfKeys; ++i) {
		keys[i].checkAndTrigger(ioDevice->readValue(keys[i].getPin()), keyCallback);
	}
}

