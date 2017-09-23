#include <LiquidCrystal.h>
#include <BasicIoAbstraction.h>

#define READ_CLOCK_PIN 28
#define READ_DATA_PIN 26
#define READ_LATCH_PIN 25
#define READ_CLK_ENA_PIN 27

#define WRITE_CLOCK_PIN 22
#define WRITE_DATA_PIN 23
#define WRITE_LATCH_PIN 24

BasicIoAbstraction* shiftRegister = inputOutputFromShiftRegister(READ_CLOCK_PIN, READ_DATA_PIN, READ_LATCH_PIN, READ_CLK_ENA_PIN, WRITE_CLOCK_PIN, WRITE_DATA_PIN, WRITE_LATCH_PIN);

void setup() {
	// although not technically needed for the shift register we should always call pinDirection
	// as it makes it possible to switch in future to either arduino direct or IO expander.

	// 0-7 are always input and 24 onwards are always output with the shift register abstraction
	// this allows for a future improvement where more than one register could be easily supported.
	for (int i = 24; i < 32; ++i) {
		shiftRegister->pinDirection(i, OUTPUT);
	}
	for (int i = 0; i < 8; ++i) {
		shiftRegister->pinDirection(i, INPUT);
	}
	

}

uint8_t counter = 0;

void loop() {
	delay(1);
	shiftRegister->runLoop();
	shiftRegister->writeValue(24, shiftRegister->readValue(1));
	shiftRegister->writeValue(25, shiftRegister->readValue(2));
	shiftRegister->writeValue(26, shiftRegister->readValue(3));
	counter++;
	shiftRegister->writeValue(27, (counter > 128));
}