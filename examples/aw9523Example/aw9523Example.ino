/**
 * This shows a simple example of interfacing with an AW9523 I2C device using IoAbstraction. This example shows all
 * three main use cases:
 *
 * * Creating a switch using switches - pin 0
 * * Switch an output on and off - pin 9
 * * Shows wrapping an analog abstraction and using the LED controller mode - pin 8
 */
#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <IoLogging.h>

// First create the overall I2C device driver
AW9523IoAbstraction ioDevice(0x58, IO_PIN_NOT_DEFINED);
// Now we create an Analog wrapper object for simpler LED controller behaviour
AW9523AnalogAbstraction ledController(ioDevice);

uint8_t lastVal = 0;
int8_t direction = 1;
bool lastBool = false;

IOLOG_MBED_PORT_IF_NEEDED(USBTX, USBRX);

void setup() {
    IOLOG_START_SERIAL

    // Set up wire for your board
    Wire.setSDA(12);
    Wire.setSCL(13);
    Wire.begin();

    // First we set an item as input.
    ioDevice.pinMode(0, INPUT);
    serdebugFHex("Device ID: ", (int)ioDevice.deviceId());

    // set the current to half power or about 18.5mA.
    ioDevice.writeGlobalControl(true, AW9523IoAbstraction::CURRENT_HALF);

    // Next we initialise the analog pin for LED control, then create a task that runs frequently to update it
    ledController.initPin(8, DIR_PWM);
    taskManager.scheduleFixedRate(1, [] {
        ledController.setCurrentValue(8, lastVal);
        lastVal = lastVal + direction;
        if(lastVal == 0) direction = 1;
        else if(lastVal == 255) direction = -1;
    });

    // Then we create the digital switch, we set as output and a task switches it frequently
    ioDevice.pinMode(9, OUTPUT);
    taskManager.scheduleFixedRate(100, [] {
        ioDevice.digitalWriteS(9, lastBool);
        lastBool = !lastBool;
    });
}

void loop() {
    taskManager.runLoop();
}

#ifdef IOA_USE_MBED
int main() {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
#endif