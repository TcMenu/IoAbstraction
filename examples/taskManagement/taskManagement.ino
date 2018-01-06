/*
TaskManagement Example for BasicIoAbstraction library

TaskManager is designed to replace the standard delay() then do something pattern in Arduino code.
When using this library, we express waits differently, more in line with in 10 seconds turn
on the led. To do this we use a scheduler that controls when tasks are done. For each task
that we schedule, we provide a function that will be called upon that schedule. This example
shows all the uses of the scheduler, for a simpler example, see the timedBlink example.

To test the interrupt support, wire a switch to pin 2 with pull up/down. Each change of state
will cause an interrupt.

Written by Dave Cherry of thecoderscorner.com in 2017
Licenced with an Apache licnese.

*/

#include <Wire.h>
#include <BasicIoAbstraction.h>

char slotString[10] = { 0 };

int taskId = -1;

void setup() {
	pinMode(2, INPUT);

	Serial.begin(9600);
	Serial.println("Starting task manager");
	taskManager.scheduleOnce(10000, tenSecondsUp);
	taskId = taskManager.scheduleFixedRate(1000, oneSecondPulse);
	
	log("Waiting 32 milli second with yield in setup");
	taskManager.yieldForMicros(32000);
	log("Waited 32 milli second with yield in setup");

	taskManager.scheduleOnce(30000, [] {
		taskManager.cancelTask(taskId); 
	});
	taskManager.scheduleFixedRate(5, [] { 
		log(taskManager.checkAvailableSlots(slotString)); 
	}, TIME_SECONDS);
	taskManager.scheduleFixedRate(100, onMicrosJob, TIME_MICROS);
	taskManager.setInterruptCallback (onInterrupt);
	taskManager.addInterrupt(2, CHANGE);

	taskManager.scheduleFixedRate(10, [] { taskManager.yieldForMicros(10000); });
}


void onInterrupt(uint8_t bits) {	
	// notice it is safe to call Serial and log here, we are not in an interrupt handler.
	log("Interrupt triggered");
	Serial.print("  Interrupt was ");
	Serial.println(bits);
}

int microCount = 0;
void onMicrosJob() {
	microCount++;
	if ((microCount % 100) == 1) {
		log("Micros job increased by 100");
	}
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