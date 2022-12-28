//
// This simple demo shows the use of the IO abstraction library with both regular IO pins, and also
// with pins on a PCF8574, there's another example for multiIO, this just shows the basics.
//
// One caveat is that for io expansion ports, the port is only updated when runLoop is called.
// for direct arduino, this makes no difference, but you should always call it or the library
// would not work for IO expanders.
//
// There is another sketch that shows the library working with serial shift registers too.
// See: http://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/i2c8574-example-ioAbstraction-library/
//
// Documentation and reference:
//
// https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
// https://www.thecoderscorner.com/ref-docs/ioabstraction/html/index.html
//
// For mbed compatibility, copy the INO file into a CPP and define COMPILE_FOR_MBED.

#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <TaskManagerIO.h>
#include <IoLogging.h>

// uncomment this for mbed, comment out for Arduino.
//#define COMPILE_FOR_MBED
#ifdef COMPILE_FOR_MBED
#include <mbed.h>
#else
#include <Wire.h>
#endif

#define INPUT_PIN 0
#define OUTPUT_PIN 12

// this sets up logging on mbed but is ignored on Arduino, so you can leave in place for both.
IOLOG_MBED_PORT_IF_NEEDED(USBTX, USBRX);

// Here we create a simple expander to access a PCF8574, address is 0x27, interrupt pin not defined, using regular Wire.
PCF8574IoAbstraction ioExpander(0x27, IO_PIN_NOT_DEFINED, &Wire);
BasicIoAbstraction ioBoard = internalDigitalDevice();

// remember the last state of the IO expander switch so we can log any changes later
uint8_t lastSwitchIoExp = 0;

void setup() {
    // This example logs using IoLogging, see the following guide to enable
    // https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/arduino-logging-with-io-logging/
    IOLOG_START_SERIAL

	// here we set the direction of pins on the IO expander
	ioExpander.pinMode(INPUT_PIN, INPUT);
	ioBoard.pinMode(OUTPUT_PIN, OUTPUT);
}

void loop() {

	// we must always call the read loop, this allows the library to send the i2c command to the device, avoiding a call 
	// with every adjustment. If you're only doing one read / write, use ioDeviceDigitalReadS or ioDeviceDigitalWriteS instead
	// as they sync automatically, but less efficiently for many calls at once.
	ioExpander.sync();

	// here we read from the IO expander and write to serial.
	uint8_t newSwitchIoExp = ioExpander.digitalRead(INPUT_PIN);
	if (newSwitchIoExp != lastSwitchIoExp) {
		serdebugF2("Switch 0 pressed on IO expander: ", newSwitchIoExp);
        ioBoard.digitalWrite(OUTPUT_PIN, newSwitchIoExp);
	}
	lastSwitchIoExp = newSwitchIoExp;
}

#ifdef COMPILE_FOR_MBED
int main() {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
#endif