#include "mbed.h"
#include "rtos.h"
#include <IoLogging.h>
#include <TaskManager.h>
#include <SwitchInput.h>
#include <JoystickSwitchInput.h>
#include <AnalogDeviceAbstraction.h>

int myCount = 0;

// to be able to use IoLogging within your application add the following
Serial console(USBTX, USBRX);
MBedLogger LoggingPort(console);
IoAbstractionRef ioRef = internalDigitalIo();
MBedAnalogDevice analogDevice;

volatile bool exitApp = false;

void buttonPressed(pinid_t pin, bool held) {
    serdebugF3("Button pressed: ", pin, held)
}

void buttonReleased(pinid_t pin, bool held) {
    serdebugF3("Button released: ", pin, held)
}

void doSomeLogging() {// IoAbstraction includes a very basic logging framework, here we write a few debug statements out
// See IoLogging.h for usage and options. It should NEVER be left ENABLED in production code.
// When not enabled it is compiled out of your code.
    serdebugF2("Debug double ", 15.943);
    serdebugFHex("Debug hex1 ", 255);
    serdebugFHex2("Debug hex2 ", 255, 0xaa);
    serdebugF4("Debug mixed: ", 255, "something", true);
}

class ExecutableTask : public Executable {
private:
    int someSharedState;
public:
    ExecutableTask(int someSharedState) : someSharedState(someSharedState) {}

    void exec() override {
        serdebugF2("exec() shared state: ", someSharedState);
    }
};
ExecutableTask myTask(100);

void scheduleSomeTasks() {
    taskManager.scheduleFixedRate(1, [] {
        serdebugF2("Second counter: ", myCount);
        ioDeviceDigitalWriteS(ioRef, LED1, (myCount % 2) == 0);
    }, TIME_SECONDS);

    taskManager.scheduleFixedRate(100, [] {
        myCount++;
    }, TIME_MILLIS);

    taskManager.scheduleOnce(5, [] {
        taskManager.scheduleOnce(20, &myTask, TIME_SECONDS);
    }, TIME_SECONDS);
}

int main() {
    console.baud(115200);

    ioDevicePinMode(ioRef, LED1, OUTPUT);
    analogDevice.initPin(A0, DIR_IN);

    doSomeLogging();

    scheduleSomeTasks();

    switches.initialise(ioRef, false);
    switches.addSwitch(USER_BUTTON, buttonPressed, 20);
    switches.onRelease(USER_BUTTON, buttonReleased);
    switches.addSwitch(D8, buttonPressed, NO_REPEAT, true);

    setupAnalogJoystickEncoder(&analogDevice, A0, [](int value) {
        serdebugF2("JoystickValue: ", value);
    });
    switches.getEncoder()->changePrecision(250, 125);

    while(!exitApp) {
        taskManager.runLoop();
    }
}
