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

#include <Wire.h>
#include <IoAbstraction.h>
#include <IoLogging.h>
#include <TaskManagerIO.h>
#include <IoAbstractionWire.h>

//
// we normally try and group input and output on different ports, it is more efficient and
// works better under load.
//

// the pins where the encoder is connected
const int encoderA = 6;
const int encoderB = 7;
const int encoderOK = 5;

// the pins where the LEDs are connected
const int ledA = 8;
const int ledB = 9;

// Arduino 23017 interrupt pin connection, and reset pin connection
const int arduinoInterruptPin = 2;
const int resetPin23017 = 32;

IoAbstractionRef io23017 = ioFrom23017(0x20, ACTIVE_LOW_OPEN, arduinoInterruptPin);

//
// this function is called by switches whenever the button is pressed.
//
void onKeyPressed(pinid_t key, bool held) {
    serdebugF("key pressed");
    ioDeviceDigitalWrite(io23017, ledA, HIGH);
    ioDeviceDigitalWriteS(io23017, ledB, HIGH);
}

void onKeyReleased(pinid_t key, bool held) {
    ioDeviceDigitalWrite(io23017, ledA, LOW);
    ioDeviceDigitalWriteS(io23017, ledB, LOW);
}

void onEncoderChange(int encoderValue) {
    serdebugF2("encoder = ", encoderValue);
    ioDeviceDigitalWrite(io23017, ledA, encoderValue < 0);
    ioDeviceDigitalWriteS(io23017, ledB, encoderValue > 0);
}

void setup() {
    // startup wire and serial.
    Wire.begin();
    Serial.begin(115200);

    // this is optional, in a real world system you could probably just connect the
    // reset pin of the device to Vcc, but when prototyping you'll want a reset
    // on every restart.
    pinMode(resetPin23017, OUTPUT);
    digitalWrite(resetPin23017, LOW);
    delayMicroseconds(100);
    digitalWrite(resetPin23017, HIGH);

    serdebugF("Starting LED example on 23017 example");

    ioDevicePinMode(io23017, ledA, OUTPUT);
    ioDevicePinMode(io23017, ledB, OUTPUT);

    switches.initialiseInterrupt(io23017, true);
    switches.addSwitch(encoderOK, onKeyPressed, 20);
    switches.onRelease(encoderOK, onKeyReleased);
    setupRotaryEncoderWithInterrupt(encoderA, encoderB, onEncoderChange);
}

void loop() {
    // switches needs task manager to run, we must therefore call it every loop and avoid using delays,
    // instead schedule stuff to be done.
    taskManager.runLoop();
}