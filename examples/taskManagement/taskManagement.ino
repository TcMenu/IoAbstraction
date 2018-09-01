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

#include <IoAbstraction.h>

// We'll register an interrupt on an arduino pin later, so need this reference. 
IoAbstractionRef arduinoIo = ioUsingArduino();

// we use this to provide the debug information that shows the state of each task slot
char slotString[10] = { 0 };

int taskId = -1;

void log(const char* logLine) {
	Serial.print(millis());
	Serial.print(": ");
	Serial.println(logLine);
}


void setup() {
	pinMode(2, INPUT);

	setMillis(-9000);

	Serial.begin(9600);
	Serial.println("Starting task manager");
	taskManager.scheduleOnce(10000, tenSecondsUp);
	taskId = taskManager.scheduleFixedRate(1000, oneSecondPulse);
	
	// first we do a yield operation for 32 millis to test the yield functionality
	log("Waiting 32 milli second with yield in setup");
	taskManager.yieldForMicros(32000);
	log("Waited 32 milli second with yield in setup");

	// now schedule a task to run once in 30 seconds
	taskManager.scheduleOnce(30000, [] {
		log("30 seconds up, stopping 1 second job");
		taskManager.cancelTask(taskId); 
	});

	// and another to run repeatedly at 5 second intervals, shows the task slot status
	taskManager.scheduleFixedRate(5, [] { 
		log(taskManager.checkAvailableSlots(slotString)); 
	}, TIME_SECONDS);

	// and now a job that runs every 100 micros
	taskManager.scheduleFixedRate(100, onMicrosJob, TIME_MICROS);

	// and one every 10 millis that yields for a while.
	taskManager.scheduleFixedRate(10, [] { taskManager.yieldForMicros(10000); });

	// register a port 2 interrupt.
	taskManager.setInterruptCallback (onInterrupt);
	taskManager.addInterrupt(arduinoIo, 2, CHANGE);
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
	if (abs(microCount % 100) == 1) {
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

void loop() {
	taskManager.runLoop();
}

#ifdef __AVR__
#include <util/atomic.h>
void setMillis(unsigned long ms)
{
  extern unsigned long timer0_millis;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    timer0_millis = ms;
  }
}
#else
void setMillis(unsigned long) {}
#endif
