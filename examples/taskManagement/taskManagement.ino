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
*/

#include <IoAbstraction.h>

// We'll register an interrupt on an arduino pin later, so need this reference. 
IoAbstractionRef arduinoIo = ioUsingArduino();

// we use this to provide the debug information that shows the state of each task slot
char slotString[20] = { 0 };

int taskId = -1;

/**
 * We'll use this function to print out the milliseconds from start and also the
 * line to write to Serial
 */
void log(const char* logLine) {
	Serial.print(millis());
	Serial.print(": ");
	Serial.println(logLine);
}

void setup() {
	// start up serial, the second line is for 32 bit boards.
	Serial.begin(115200);
	while(!Serial);

	log("Starting task manager");

	// connect a switch to pin 2, so you can raise interrupts.
	pinMode(2, INPUT);

	// We schedule the function tenSecondsUp() to be called in 10,000 milliseconds.
	taskManager.scheduleOnce(10000, tenSecondsUp);
	
	// Now we schedule oneSecondPulse() to be called every second. 
	// keep hold of the ID as we will later cancel it from running.
	taskId = taskManager.scheduleFixedRate(1, oneSecondPulse, TIME_SECONDS);

	//
	// now we do a yield operation, which is similar to delayMicroseconds but allows other
	// tasks to be run during that time.
	//
	log("Waiting 32 milli second with yield in setup");
	taskManager.yieldForMicros(32000);
	log("Waited 32 milli second with yield in setup");

	// now schedule a task to run once in 30 seconds
	taskManager.scheduleOnce(30000, [] {
		log("30 seconds up, stopping 1 second job");

		// now cancel the one second job we scheduled earlier
		taskManager.cancelTask(taskId); 
	});

	// and another to run repeatedly at 5 second intervals, shows the task slot status
	taskManager.scheduleFixedRate(5, [] { 
		log(taskManager.checkAvailableSlots(slotString)); 
	}, TIME_SECONDS);

	// and now schedule onMicrosJob() to be called every 100 micros
	taskManager.scheduleFixedRate(100, onMicrosJob, TIME_MICROS);

	// and lastly a job that schedules in microseconds.
	taskManager.scheduleFixedRate(10, [] { taskManager.yieldForMicros(10000); });

	// register a port 2 interrupt.
	taskManager.setInterruptCallback (onInterrupt);
	taskManager.addInterrupt(arduinoIo, 2, CHANGE);
}

/**
 * This is called by taskManager when the interrupt is raised. TaskManager marshalls the
 * interrupt into a task, so it is safe to call Serial etc here. Be aware that interrupts
 * handled by taskManager are not completely real time, so only use when some slight delay
 * can be accepted. 
 * 
 * - Safe usage: change in rotary encoder, button pressed.
 * - Unsafe usage: over temprature shutdown, safety circuit.
 */
void onInterrupt(uint8_t bits) {	
	log("Interrupt triggered");
	Serial.print("  Interrupt was ");
	Serial.println(bits);
}

// count up the number of times the micros job has been called.
int microCount = 0;

/**
 * Called by taskManager - this is the microsecond job registered in setup.
 * We just count up the number of calls and log it every 100 calls.
 */
void onMicrosJob() {
	microCount++;
	if (abs(microCount % 100) == 1) {
		log("Micros job increased by 100");
	}
}

/**
 * Called by taskManager - this is the one second job registered in setup
 */
void oneSecondPulse() {
	log("One second pulse");
}

/**
 * Called by taskManager when ten seconds is up, the job is registered in setup.
 * It starts another job in 10 seconds time, the 20 second job.
 */
void tenSecondsUp() {
	log("Ten seconds up");
	taskManager.scheduleOnce(10000, twentySecondsUp);
}

/**
 * This is the job that was started in the tenSecondsUp above.
 */
void twentySecondsUp() {
	log("Twenty seconds timer");
}

/**
 * The loop method must contain the taskManager.runLoop() call, it
 * checks all the schedules and interrupt states and runs any tasks
 * that are due.
 */
void loop() {
	taskManager.runLoop();
}
