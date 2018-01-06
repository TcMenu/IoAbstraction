// This is the simplest possible example of using IO Abstraction
// To directly access arduino pins. Just connect a PULL UP/DOWN 
// switch to the input pin and it will be mirrored to the LED.
// By default the output uses the inbuilt LED.

#include <BasicIoAbstraction.h>

IoAbstractionRef arduinoPins = ioUsingArduino();

void setup() {
	// and also on arduino pins
	ioDevicePinMode(arduinoPins, 10, INPUT);
	ioDevicePinMode(arduinoPins, 13, OUTPUT);
}

void loop() {
	// read the arduino, write to IO expander.
	uint8_t switchValue = ioDeviceDigitalRead(arduinoPins, 30);
	ioDeviceDigitalWrite(arduinoPins, 31, switchValue);
	ioDeviceSync(arduinoPins);
}