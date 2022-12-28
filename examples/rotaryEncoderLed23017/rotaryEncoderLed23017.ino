/**
 * This example shows how to use an MCP23017 with interrupt support to control two LEDs
 * by turning a rotary encoder. If the direction is up, the first LED lights, if the
 * direction is down the other lights. When the button is down both LEDs light, and
 * turn off upon release.
 *
 * The interrupt support for these devices works by connecting the interrupt pin of
 * the device to an interrupt capable pin on the device. Further, you must ensure
 * that reset is held in the right state, otherwise instability will result.
 *
 * Documentation and reference:
 *
 * https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
 * https://www.thecoderscorner.com/ref-docs/ioabstraction/html/index.html
 *
 */

#include <IoAbstraction.h>
#include <IoLogging.h>
#include <TaskManagerIO.h>
#include <IoAbstractionWire.h>

// This line is ignored on Arduino, it initialises the USB serial port on mbed boards.
IOLOG_MBED_PORT_IF_NEEDED(USBTX, USBRX);

//
// we normally try and group input and output on different ports, it is more efficient and
// works better under load. Here we define the pins on which the encoder and LEDs are attached
// to the MCP23017 device.
//
const int encoderA = 6;
const int encoderB = 7;
const int encoderOK = 5;
const int ledA = 8;
const int ledB = 9;

// Also, we define the Arduino pin on which the I2C devices interrupt pin connects to the board.
const int attachedInterruptPin = 2;

// now we define the actual device driver, providing these parameters:
// 1. the I2C address in device format, on Arduino that is generally the 7 bit address.
// 2. interrupt mode is one of: NOT_ENABLED, ACTIVE_HIGH_OPEN, ACTIVE_LOW_OPEN, ACTIVE_HIGH, ACTIVE_LOW
// 3. attached interrupt pin for port A (or port A and B if port B is not defined), or IO_PIN_NOT_DEFINED for none
// 4. attached interrupt pin for port B or IO_PIN_NOT_DEFINED
MCP23017IoAbstraction mcp23017(0x20, ACTIVE_LOW_OPEN,  attachedInterruptPin, IO_PIN_NOT_DEFINED);

//
// called whenever the button on the encoder is pressed, see the switches.addSwitch below
//
void onKeyPressed(pinid_t key, bool held) {
    serdebugF3("key pressed", key, held);
    mcp23017.digitalWrite(ledA, HIGH);
    mcp23017.digitalWriteS(ledB, HIGH);
}

//
// called whenever the button on the encoder is released, see the switches.onRelease below
//
void onKeyReleased(pinid_t key, bool held) {
    serdebugF3("key released", key, held);
    mcp23017.digitalWrite(ledA, LOW);
    mcp23017.digitalWriteS(ledB, LOW);
}

//
// called whenever the encoder value changes, see the encoder creation below
//
void onEncoderChange(int encoderValue) {
    serdebugF2("encoder = ", encoderValue);
    mcp23017.digitalWrite(ledA, encoderValue < 0);
    mcp23017.digitalWriteS(ledB, encoderValue > 0);
}

void setup() {
    // This example logs using IoLogging, see the following guide to enable
    // https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/arduino-logging-with-io-logging/
    IOLOG_START_SERIAL

    // IMPORTANT: Initialise wire, many boards will hang if this is missing
    Wire.begin();
//    Wire.begin(4, 15); // for ESP devices

    // this is optional, in a real world system you could probably just connect the reset pin of the device to Vcc,
    // but when prototyping you'll want a reset on every restart.
    const int resetPin23017 = 32;
    mcp23017.resetDevice(resetPin23017);

    serdebugF("Starting LED example on 23017 example");

    // In the example two LEDs are attached to the device, on ledA and ledB pins. Here we set them as output on the
    // mcp23017 device itself.
    mcp23017.pinMode(ledA, OUTPUT);
    mcp23017.pinMode(ledB, OUTPUT);

    // our next task is to initialise switches, do this BEFORE doing anything else with switches.
    // We choose to initialise in poll everything (requires no interrupts), but there are other modes too:
    // (SWITCHES_NO_POLLING - interrupt only) or (SWITCHES_POLL_KEYS_ONLY - encoders on interrupt)
    switches.init(asIoRef(mcp23017), SWITCHES_NO_POLLING, true);

    // we now add both a press and release and handler.
    switches.addSwitch(encoderOK, onKeyPressed, 20);
    switches.onRelease(encoderOK, onKeyReleased);

    // and set up an encoder on the same device.
    setupRotaryEncoderWithInterrupt(encoderA, encoderB, onEncoderChange);

    // here we set the range and current value of the encoder, if both values are 0, then the encoder goes into
    // direction only mode, IE -1 for down, +1 for up.
    const int maximumValue = 1000;           // the absolute maximum value, from 0..maximum
    const int currentValue = 50;            // sets the current value
    const uint8_t stepRate = 1;             // how much to advance on each position
    const bool wrapAroundAtMinMax = false;  // if we should wrap from max back to 0, 0 to max.
    switches.changeEncoderPrecision(0, maximumValue, currentValue, wrapAroundAtMinMax, stepRate);

    // instead of the above changeEncoderPrecision call, you could have a direction only encoder where down is reported
    // as -1 and 1 for up.
    //switches.getEncoder()->setUserIntention(DIRECTION_ONLY);
}

void loop() {
    // switches needs task manager to run, we must therefore call it every loop and avoid using delays,
    // instead schedule stuff to be done.
    taskManager.runLoop();
}

