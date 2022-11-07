/**
 * An example of how to use the MultiIoAbstraction support in this library
 *
 * MultiIoAbstraction provides support for using more than one type of IO at once
 * with the same IoAbstraction. For example if one needed to connect many switches
 * to a single switches instance, you may need several IoExapnders and the Arduino
 * pins as well.
 *
 * To use this library we simply tell the device how many pins the Arduino uses
 * (otherwise it guesses at 100). Then above that number, each Io device that
 * you add uses additional pins.
 *
 * In this example we use IoAbstraction's switches with a multiIo that is based
 * on Arduino pins and a PCF8574 too, we read a switch and set some LEDs
 *
 * More information:
 * https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/arduino-pins-and-io-expanders-same-time/
 *
 */


#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include<TaskManagerIO.h>
#include <Wire.h>

// to make life easier, probably define each expander
#define EXPANDER1 100

// create a multi Io that allocates the first 100 pins to arduino pins
MultiIoAbstraction multiIo(EXPANDER1);
PCF8574IoAbstraction ioExpander(0x20, IO_PIN_NOT_DEFINED);

//
// when the switch is pressed then this function will be called.
//
void onSwitchPressed(uint8_t key, bool held) {
    // here we just toggle the state of the built in LED and an LED on the expander.
    uint8_t ledState = multiIo.digitalReadS(LED_BUILTIN);

    multiIo.digitalWrite(LED_BUILTIN, !ledState);
    multiIo.digitalWrite(EXPANDER1 + 1, !ledState);
    multiIo.sync(); // force another sync

    Serial.print("Switch ");
    Serial.print(key);
    Serial.println(held ? " Held down" : " Pressed");
}

//
// traditional arduino setup function
//
void setup() {
    Wire.begin();
    Serial.begin(115200);

    Serial.println("Multi IoExpander example");

    // we now add an 8574 chip that allocates 10 more pins, therefore it goes from 100..109
    multiIo.addIoDevice(io8574, 10);
    // add more expanders here..

    Serial.println("added an expander at pin 100 to 109");

    // set up the outputs we are going to use which are basically
    // the built in LED
    // port 1 of the expander.
    multiIo.pinMode(LED_BUILTIN, OUTPUT);
    multiIo.pinMode(EXPANDER1 + 1, OUTPUT);

    Serial.println("Io is setup, adding switch");

    // set up the button on port 0 of the expander. we choose poll everything (including encoders here) but you could
    // also SWITCHES_POLL_KEYS_ONLY and SWITCHES_NO_POLLING for interrupt mode.
    switches.init(asIoRef(multiIo), SWITCHES_POLL_EVERYTHING, true);

    switches.addSwitch(EXPANDER1 + 0, onSwitchPressed);
    multiIo.pinMode(EXPANDER1, INPUT);

    Serial.println("setup is done!");
}

void loop() {
    taskManager.runLoop();
}
