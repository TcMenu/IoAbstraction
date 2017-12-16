#include <inttypes.h>
#include <Arduino.h>
#include "SwitchInput.h"

SwitchInput* SwitchInput::__swInInstance;


KeyboardItem::KeyboardItem() {
	this->repeatInterval = NO_REPEAT;
	this->pin = -1;
	this->callback = NULL;
}

void KeyboardItem::initialise(uint8_t pin, KeyCallbackFn callback, uint8_t repeatInterval = NO_REPEAT) {
	this->callback = callback;
	this->pin = pin;
	this->repeatInterval = repeatInterval;
	state = NOT_PRESSED;
}

void KeyboardItem::checkAndTrigger(uint8_t buttonState){
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
	this->callback = callback;
	pinA = 0xff;
	pinB = 0xff;
	currentReading = 0;
}

void RotaryEncoder::initialise(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback) {
	this->pinA = pinA;
	this->pinB = pinB;

	this->currentReading = 0;
	this->maximumValue = 0;

	this->callback = callback;
}

void RotaryEncoder::changePrecision(uint16_t maxValue, int currentValue) {
	this->maximumValue = maxValue;
	this->currentReading = currentValue;
	callback(currentReading);
}


void RotaryEncoder::encoderChanged(IoAbstractionRef ioDev) {
	ioDeviceSync(ioDev);
	if (ioDeviceDigitalRead(ioDev, pinA) != LOW) {
		if (ioDeviceDigitalRead(ioDev, pinB) == LOW) {
			currentReading = (currentReading != 0) ? --currentReading : 0;
		}
		else {
			currentReading = min(++currentReading, maximumValue);
		}		

		callback(currentReading);
	}
}

SwitchInput::SwitchInput() {
	this->numberOfKeys = 0;	
}

void SwitchInput::initialise(TaskManager& taskManager, IoAbstractionRef ioDevice) {
	this->ioDevice = ioDevice;

	__swInInstance = this;
	taskManager.scheduleFixedRate(20, [] {
		SwitchInput::__swInInstance->runLoop();
	});

}

void SwitchInput::addSwitch(uint8_t pin, KeyCallbackFn callback,uint8_t repeat = NO_REPEAT) {
	keys[numberOfKeys++].initialise(pin, callback, repeat);
	ioDevice->pinDirection(pin, INPUT);
}

void SwitchInput::initialiseEncoder(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback) {

	ioDevicePinMode(ioDevice, pinA, INPUT);
	ioDevicePinMode(ioDevice, pinB, INPUT);

	encoder.initialise(pinA, pinB, callback);
}

void SwitchInput::changeEncoderPrecision(uint16_t precision, uint16_t currentValue) {
	encoder.changePrecision(precision, currentValue);
}

void SwitchInput::runLoop() {
	ioDevice->runLoop();
	for (int i = 0; i < numberOfKeys; ++i) {
		keys[i].checkAndTrigger(ioDevice->readValue(keys[i].getPin()));
	}
}

void SwitchInput::encoderChanged() {
	encoder.encoderChanged(ioDevice);
}

void onEncoderInterrupt() {
	SwitchInput::__swInInstance->encoderChanged();
}

