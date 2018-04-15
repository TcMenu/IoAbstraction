#include <inttypes.h>
#include <Arduino.h>
#include "SwitchInput.h"

SwitchInput switches;

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

SwitchInput::SwitchInput() {
	this->numberOfKeys = 0;	
}

void SwitchInput::initialise(IoAbstractionRef ioDevice) {
	this->ioDevice = ioDevice;

	taskManager.scheduleFixedRate(20, [] {
		switches.runLoop();
	});

}

void SwitchInput::addSwitch(uint8_t pin, KeyCallbackFn callback,uint8_t repeat = NO_REPEAT) {
	keys[numberOfKeys++].initialise(pin, callback, repeat);
	ioDevice->pinDirection(pin, INPUT);
}

void SwitchInput::setEncoder(RotaryEncoder* theEncoder) {
	encoder = theEncoder;
}

void SwitchInput::changeEncoderPrecision(uint16_t precision, uint16_t currentValue) {
	if(encoder != NULL) {
		encoder->changePrecision(precision, currentValue);
	}
}

void SwitchInput::runLoop() {
	ioDevice->runLoop();
	for (int i = 0; i < numberOfKeys; ++i) {
		keys[i].checkAndTrigger(ioDevice->readValue(keys[i].getPin()));
	}
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

HardwareRotaryEncoder::HardwareRotaryEncoder(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback) : RotaryEncoder(callback) {
	this->pinA = pinA;
	this->pinB = pinB;
	this->debounceFlags = 0;

	pinMode(pinA, INPUT_PULLUP);
	pinMode(pinB, INPUT_PULLUP);
}

void onEncoderInterrupt(uint8_t pin) {
	((HardwareRotaryEncoder*)switches.encoder)->encoderChanged();
}

void onDebounceAction() {
	((HardwareRotaryEncoder*)switches.encoder)->debounceAction();
}

void HardwareRotaryEncoder::debounceAction() {
	bool locPinA = (debounceFlags & PINA_FLAG) != 0;
	if (digitalRead(pinA) != locPinA) {
		if (debounceFlags & PIN_DEBOUNCE2) {
			// clear everything down and assume it was an error, still not settled in 3 ms.
			debounceFlags = 0;
		}
		else {
			debounceFlags &= DEBOUCEFLAGS;
			debounceFlags |= PIN_DEBOUNCE2;
			taskManager.scheduleOnce(1, onDebounceAction);
		}
	}
	else {
		// we need pinB from the time when we started debouncing.
		bool locPinB = (debounceFlags & PINB_FLAG) != 0;
		if (locPinB) {
			currentReading = min(currentReading + 1, maximumValue);
		}
		else {
			currentReading = (currentReading != 0) ? currentReading - 1 : 0;
		}

		callback(currentReading);
		debounceFlags = 0;
	}
}

void HardwareRotaryEncoder::encoderChanged() {
	// pin A is high, interrupt is on rising edge
	if ((debounceFlags & DEBOUCEFLAGS) == 0) {
		debounceFlags = PINA_FLAG;
		debounceFlags |= PIN_DEBOUNCE1;
		if(digitalRead(pinB)) {
			debounceFlags |= PINB_FLAG;
		}
		taskManager.scheduleOnce(1, onDebounceAction);
	}
}

/******** UP DOWN BUTTON ENCODER *******/

void switchEncoderUp(uint8_t key, bool heldDown) {
	((EncoderUpDownButtons*)switches.encoder)->increment(1);
}

void switchEncoderDown(uint8_t key, bool heldDown) {
	((EncoderUpDownButtons*)switches.encoder)->increment(-1);
}

EncoderUpDownButtons::EncoderUpDownButtons(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback, uint8_t speed = 5) : RotaryEncoder(callback) {
	Serial.begin(9600);
	switches.addSwitch(pinUp, switchEncoderUp, speed);
	switches.addSwitch(pinDown, switchEncoderDown, speed);
}

void EncoderUpDownButtons::increment(int8_t incVal) { 
	if ((currentReading + incVal) < 0) return;
	if ((currentReading + incVal) > maximumValue) return;

	currentReading += incVal; callback(currentReading);
}

/******** ENCODER SETUP METHODS ***********/

void setupUpDownButtonEncoder(uint8_t pinUp, uint8_t pinDown, EncoderCallbackFn callback) {
	EncoderUpDownButtons* enc = new EncoderUpDownButtons(pinUp, pinDown, callback);
	switches.setEncoder(enc);
}

void setupRotaryEncoderWithInterrupt(uint8_t pinA, uint8_t pinB, EncoderCallbackFn callback) {
	switches.setEncoder(new HardwareRotaryEncoder(pinA, pinB, callback));
	taskManager.setInterruptCallback(onEncoderInterrupt);
	taskManager.addInterrupt(pinA, RISING);
}
