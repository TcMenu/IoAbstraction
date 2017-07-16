#include <BasicIoAbstraction.h>
#include <Wire.h>

//
// This simple demo shows the use of the IO abstraction library to use both a 8574 chip 
// and arduino pins for input and output. Normally, code would be configured to use one
// or the other, but for demonstration, I show both in the same sketch.
//
// The method calls are very similar to using pins direct, and if you use
// ioUsingArduino() it will use arduino pins, and ioFrom8754() uses an IO Expander at the
// address specified.
//
// This allows libraries to offer both options with minimal overhead.
//
// One caveat is that for io expansion ports, the port is only updated when runLoop is called.
// for direct arduino, this makes no difference, but you should always call it or the library
// would not work for IO expanders.
//
// Many other IO expanders could easily be added using the same interface.
//

BasicIoAbstraction* ioExpander = ioFrom8754(0x20);
BasicIoAbstraction* arduinoPins = ioUsingArduino();
uint8_t lastGreyCodeA = 0;
uint8_t lastSwitchIoExp = 0;

void setup() {
	Serial.begin(9600);

	// if using the i2c IO expander we must make sure Wire is initialised.
	// This would not normally be done in library code, but by the callee.
	Wire.begin();

	// here we set the direction of pins on the IO expander
	ioExpander->pinDirection(0, INPUT);
	ioExpander->pinDirection(6, OUTPUT);

	// and also on arduino pins
	arduinoPins->pinDirection(30, INPUT);
	arduinoPins->pinDirection(28, INPUT);
	arduinoPins->pinDirection(29, INPUT);
}

void loop() {

	// read the arduino, write to IO expander.
	uint8_t switchValue = arduinoPins->readValue(30);
	ioExpander->writeValue(6, switchValue);
	
	// we must always call the read loop, this allows the library to send the i2c command
	// to the device, avoiding a call with every adjustment.
	ioExpander->runLoop();
	arduinoPins->runLoop();

	// here we read from the IO expander and write to serial.
	uint8_t newSwitchIoExp = ioExpander->readValue(0);
	if (newSwitchIoExp != lastSwitchIoExp) {
		Serial.print("Switch 0 pressed on IO expander:");
		Serial.println(newSwitchIoExp);
	}
	lastSwitchIoExp = newSwitchIoExp;
}
