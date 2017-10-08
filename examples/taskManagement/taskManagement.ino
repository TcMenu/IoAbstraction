/*
TaskManagement Example for BasicIoAbstraction library

TaskManager is designed to replace the standard delay().. do something pattern in Arduino code.
When using this library, we'd express waits differently, more in line with in 10 seconds turn
on the led. To do this we use a scheduler that controls when tasks are done. For each thing
that we schedule, we provide a function that will be called upon that schedule. This example
shows all the uses of the scheduler, for a simpler example, see the scheduledBlink example.

Written by Dave Cherry of thecoderscorner.com in 2017
Licenced with an Apache licnese.

*/

#include <Wire.h>
#include <TaskManager.h>

TaskManager taskManager;

char slotString[10] = { 0 };

void setup() {
	pinMode(2, INPUT);

	Serial.begin(9600);
	Serial.println("Starting task manager");
	taskManager.scheduleOnce(10000, tenSecondsUp);
	taskManager.scheduleFixedRate(1000, oneSecondPulse);
	taskManager.scheduleFixedRate(5000, [] { log(taskManager.checkAvailableSlots(slotString)); });
	taskManager.setInterruptCallback (onInterrupt);
	taskManager.addInterrupt(2, CHANGE);
}

void onInterrupt(uint8_t bits) {	
	log("Interrupt triggered");
	Serial.print("  Interrupt was ");
	Serial.println(bits);
}

void oneSecondPulse() {
	log("One second pulse");
}

void tenSecondsUp() {
	log("Ten seconds up");
	taskManager.scheduleOnce(10000, twentySecondsUp);
}

void twentySecondsUp() {
	log("Twenty seconds timer");
	taskManager.scheduleOnce(rand() * 100, randomJob);
}

void randomJob() {
	log("Random job ran");
	taskManager.scheduleOnce(rand(), randomJob);
}

void log(char* logLine) {
	Serial.print(millis());
	Serial.print(": ");
	Serial.println(logLine);

}

void loop() {
	taskManager.runLoop();
}