#include <BasicIoAbstraction.h>

IoAbstractionRef arduinoPins = ioUsingArduino();

void setup() {
	// and also on arduino pins
	ioDevicePinMode(arduinoPins, 30, INPUT);
	ioDevicePinMode(arduinoPins, 31, OUTPUT);
}

void loop() {
	// read the arduino, write to IO expander.
	uint8_t switchValue = ioDeviceDigitalRead(arduinoPins, 30);
	ioDeviceDigitalWrite(arduinoPins, 31, switchValue);
	ioDeviceSync(arduinoPins);
}