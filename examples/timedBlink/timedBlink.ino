/*
Timed blink, this example shows how to build the simple Blink application using both IoAbstraction
and TaskManager. This gives an example of how quickly a simple application can be made to leverage
this library.

Because this example uses IoAbstraction, the LED could be on the end of an i2c expander or even
on a shift register. Further, if you then needed a second timed action, it would be trivial to
add.

 Documentation and reference:

  https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
  https://www.thecoderscorner.com/ref-docs/ioabstraction/html/index.html

*/

#include <Wire.h>
#include<IoAbstraction.h>
#include<TaskManagerIO.h>

// Make sure the code builds.
#ifndef LED_BUILTIN
#define LED_BUILTIN 1
#endif

// constant for the pin we will use
const int ledPin = LED_BUILTIN;

// the state of the pin, we will toggle it.
int ledOn = LOW;

// create an IO abstraction, so later we could put the led on a shift register or i2c.

void setup() {
    Serial.begin(115200);
    Serial.println("1");
	// set the pin we are to use as output using the io abstraction
	internalDigitalDevice().pinMode(ledPin, OUTPUT);

	// and create the task that toggles the led every second.
	taskManager.scheduleFixedRate(1000, toggle);
    Serial.println("3");

}

// this is the call back method that gets called once a second
// from the schedule above.
void toggle() {
    Serial.println("4");

    // now we write to the device, the 'S' version of the method automatically syncs.
	internalDigitalDevice().digitalWriteS(ledPin, ledOn);

	ledOn = !ledOn; // toggle the LED state.
}


void loop() {
	// this is all we should do in loop when using task manager.
	taskManager.runLoop();
}
