#include <IoAbstraction.h>
#include <TaskManagerIO.h>

#define READ_CLOCK_PIN 28
#define READ_DATA_PIN 26
#define READ_LATCH_PIN 25

#define WRITE_CLOCK_PIN 22
#define WRITE_DATA_PIN 23
#define WRITE_LATCH_PIN 24

// here we create a shift register abstraction that has 1 input and 1 output shift register.
ShiftRegisterIoAbstraction shiftRegister(
        ShiftRegConfig(READ_CLOCK_PIN, READ_DATA_PIN, READ_LATCH_PIN, 1),
        ShiftRegConfig(WRITE_CLOCK_PIN, WRITE_DATA_PIN, WRITE_LATCH_PIN, 1));

// If you are using a 74HC165 for input, you can also use the following instead:
//ShiftRegisterIoAbstraction165In shiftIn74hc165(ShiftRegConfig(READ_CLOCK_PIN, READ_DATA_PIN, READ_LATCH_PIN, 1));

void setup() {
	// although not technically needed for the shift register we should always call pinDirection
	// as it makes it possible to switch in future to either use arduino direct or IO expander.

	// 0-31 are always input and 32 onwards are always output with the shift register abstraction
	// this allows for up to 4 input and 4 output devices to be chained together.
	for (int i = 32; i < 40; ++i) {
		shiftRegister.pinMode(i, OUTPUT);
	}
	for (int i = 0; i < 8; ++i) {
		shiftRegister.pinMode(i, INPUT);
	}
}

uint8_t counter = 0;

void loop() {
	delay(1);
	shiftRegister.sync();
    shiftRegister.digitalWrite(32, 1);//shiftRegister.digitalRead(1));
    shiftRegister.digitalWrite(33, shiftRegister.digitalRead(2));
    shiftRegister.digitalWrite(34, shiftRegister.digitalRead(3));
	counter++;
	shiftRegister.digitalWrite(35, (counter > 128));
}
