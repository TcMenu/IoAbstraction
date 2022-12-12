/**
 * This example shows how to use an MCP23017 with interrupt support to control two LEDs
 * by turning a rotary encoder. If the direction is up, the first LED lights, if the
 * direction is down the other lights. When the button is down both LEDs light, and
 * turn off upon release.
 *
 * The interrupt support for these devices works by connecting the interrupt pin of
 * the device to an interrupt capable pin on the device. Further, you must ensure
 * that reset is held in the right state, otherwise instability will result.
 */

#include <IoAbstraction.h>
#include <IoLogging.h>
#include <TaskManagerIO.h>
#include <IoAbstractionWire.h>

// uncomment the line below for compilation on mbed, comment out for Arduino

IOLOG_MBED_PORT_IF_NEEDED(USBTX, USBRX);

//
// we normally try and group input and output on different ports, it is more efficient and
// works better under load.
//

// Daves Test environment II the pins where the encoder is connected
//const int encoderA = 6;
//const int encoderB = 7;
//const int encoderOK = 5;
//const int ledA = 8;
//const int ledB = 9;
//const int attachedInterruptPin = 2;

const int encoderA = 6;
const int encoderB = 7;
const int encoderOK = 5;
const int ledA = 4;
const int ledB = 3;
const int attachedInterruptPin = 2;


// Arduino 23017 interrupt pin connection, and reset pin connection
const int resetPin23017 = 32;

MCP23017IoAbstraction mcp23017(0x20, ACTIVE_LOW_OPEN,  attachedInterruptPin, IO_PIN_NOT_DEFINED);

//
// this function is called by switches whenever the button is pressed.
//
void onKeyPressed(pinid_t key, bool held) {
    serdebugF3("key pressed", key, held);
    mcp23017.digitalWrite(ledA, HIGH);
    mcp23017.digitalWriteS(ledB, HIGH);
}

void onKeyReleased(pinid_t key, bool held) {
    serdebugF3("key released", key, held);
    mcp23017.digitalWrite(ledA, LOW);
    mcp23017.digitalWriteS(ledB, LOW);
}

void onEncoderChange(int encoderValue) {
    serdebugF2("encoder = ", encoderValue);
    mcp23017.digitalWrite(ledA, encoderValue < 0);
    mcp23017.digitalWriteS(ledB, encoderValue > 0);
}

void setup() {
    IOLOG_START_SERIAL
    Wire.begin();
    //Wire.begin(4, 15);

    // this is optional, in a real world system you could probably just connect the
    // reset pin of the device to Vcc, but when prototyping you'll want a reset
    // on every restart.
    internalDigitalDevice().pinMode(resetPin23017, OUTPUT);
    internalDigitalDevice().digitalWriteS(resetPin23017, LOW);
    taskManager.yieldForMicros(100);
    internalDigitalDevice().digitalWriteS(resetPin23017, HIGH);

    serdebugF("Starting LED example on 23017 example");

    mcp23017.pinMode(ledA, OUTPUT);
    mcp23017.pinMode(ledB, OUTPUT);

    // here we initialise switches in interrupt mode, using pull up logic by default.
    switches.init(asIoRef(mcp23017), SWITCHES_NO_POLLING, true);

    // we now add both a press and release and handler.
    switches.addSwitch(encoderOK, onKeyPressed, 20);
    switches.onRelease(encoderOK, onKeyReleased);

    // and set up an encoder on the same device.
    setupRotaryEncoderWithInterrupt(encoderA, encoderB, onEncoderChange);
    switches.changeEncoderPrecision(0, 0);
}

void loop() {
    // switches needs task manager to run, we must therefore call it every loop and avoid using delays,
    // instead schedule stuff to be done.
    taskManager.runLoop();
}

