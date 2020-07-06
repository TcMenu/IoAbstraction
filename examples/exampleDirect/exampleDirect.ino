// This is the simplest possible example of using IO Abstraction
// To directly access arduino pins. Just connect a PULL UP/DOWN 
// switch to the input pin and it will be mirrored to the LED.
// By default the output uses the inbuilt LED.

// We have a direct dependency on Wire and Arduino ships it as a library for every board
// therefore to ensure compilation we include it here.
#include <Wire.h>

#include <IoAbstraction.h>

IoAbstractionRef arduinoPins = ioUsingArduino();

const int pinInput = 2, pinLed = LED_BUILTIN;

void setup() {
	// and also on arduino pins
	ioDevicePinMode(arduinoPins, pinInput, INPUT);
	ioDevicePinMode(arduinoPins, pinLed, OUTPUT);
}

void loop() {
	uint8_t switchValue = ioDeviceDigitalReadS(arduinoPins, pinInput);
	ioDeviceDigitalWrite(arduinoPins, pinLed, switchValue);
}
