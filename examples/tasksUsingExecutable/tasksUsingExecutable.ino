/**
 * This example shows another usage of task manager to schedule calls; where a class that extends
 * Executable is called back instead of using a function call back. This allows you to store
 * state between calls much easier and is still almost as simple to use with task manager as before.
 * It has a small RAM impact (8 bytes per vtable) on AVR boards because of their memory layout.
 * However, it has negligible impact on most 32 boards.
 */

#include <IoAbstraction.h>

/**
 * We can schedule things to be done using a class that extends Executable as below.
 * This is more efficient if there is a lot of shared state and is more inline with how 
 * full fat (IE: non-embedded) tasking frameworks work anyway. First we define the class
 * that exends from Executable.
 */
class SharedState : public Executable {
private:
    /** this is our state variable */
	int state;
    char data[10];
public:
    /**
     * Construct the object as blank
     */
    SharedState() {
        data[0] = 0;
        state = 0;
    }

    /**
     * We allow callers to change the state stored in this object
     */
	void setState(int newState) {
		state = newState;
	}

    /**
     * We also allow callers to change the text stored in this object,
     * notice we COPY the string, to ensure it's still around when we
     * need it
     */
    void setData(const char* newData) {
        strncpy(data, newData, sizeof(data));
        data[sizeof(data)] = 0; // make sure zero terminated.
    }

    /**
     * This is called back by task manager when the task is scheduled.
     */
	void exec() override {
		Serial.print("State is: ");
		Serial.print(state);
        Serial.print(" and data is: ");
        Serial.println(data);
	}
};

// now we create a global instance of our class that everything can access.
// we could create as many of these as needed and schedule them appropriately
// (memory permitting of course).
SharedState state;

void setup() {
    // wait for serial and then set it up (the first statement needed on MKR boards)
    while(!Serial);
    Serial.begin(115200);

    Serial.println("Task example starting..");
    
    // set the state's data to an initial value
    state.setData("Started");

    // create a task that calls the lambda function every 100 millis, it sets the state
    taskManager.scheduleFixedRate(100, [] {state.setState(millis() % 10000); });

    // create a task that runs the function provided once in 10 seconds
    taskManager.scheduleOnce(10, [] {state.setData("Warmup"); }, TIME_SECONDS);

    // create a task that calls the function provided once in 30 seconds.
    taskManager.scheduleOnce(30, [] {state.setData("Running"); }, TIME_SECONDS);

    // create a task that calls the exec method on our SharedState object every 250 millis.
    taskManager.scheduleFixedRate(250, &state);
}

void loop() {
    // this is the method that schedules tasks to run, call very frequently in loop() and
    // do nothing that takes any time.
    taskManager.runLoop();
}