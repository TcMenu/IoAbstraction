#include <BasicIoAbstraction.h>

BasicIoAbstraction* arduinoPins = ioUsingArduino();

void setup() {
	// and also on arduino pins
	arduinoPins->pinDirection(30, INPUT);
	arduinoPins->pinDirection(31, OUTPUT);
}

void loop() {
	// read the arduino, write to IO expander.
	uint8_t switchValue = arduinoPins->readValue(30);
	arduinoPins->digitalWrite(31, switchValue);
	arduinoPins->runLoop();
}