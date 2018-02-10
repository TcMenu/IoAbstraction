#include <IoAbstraction.h>
#include <Wire.h>

//
// This simple demo shows the use of the IO abstraction library to use a 8574 i2c IO chip.
//
// One caveat is that for io expansion ports, the port is only updated when runLoop is called.
// for direct arduino, this makes no difference, but you should always call it or the library
// would not work for IO expanders.
//
// There is another sketch that shows the library working with serial shift registers too.
// See: http://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/i2c8574-example-ioAbstraction-library/


// create both an Arduino and an IO expander based IO abstraction
IoAbstractionRef ioExpander = ioFrom8754(0x20);

// remember the last state of the IO expander switch so we can log any changes later
uint8_t lastSwitchIoExp = 0;

void setup() {
	Serial.begin(9600);

	// if using the i2c IO expander we must make sure Wire is initialised.
	// This would not normally be done in library code, but by the callee.
	Wire.begin();

	// here we set the direction of pins on the IO expander
	ioDevicePinMode(ioExpander, 0, INPUT);
	ioDevicePinMode(ioExpander, 1, OUTPUT);
}

void loop() {

	// we must always call the read loop, this allows the library to send the i2c command
	// to the device, avoiding a call with every adjustment.
	ioDeviceSync(ioExpander);

	// here we read from the IO expander and write to serial.
	uint8_t newSwitchIoExp = ioDeviceDigitalRead(ioExpander, 0);
	if (newSwitchIoExp != lastSwitchIoExp) {
		Serial.print("Switch 0 pressed on IO expander:");
		Serial.println(newSwitchIoExp);
		ioDeviceDigitalWrite(ioExpander, 6, newSwitchIoExp);
	}
	lastSwitchIoExp = newSwitchIoExp;
}
