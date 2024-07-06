/*
 This sketch shows how to use a rotary encoder along with its inbuilt push button with switches.
 In addition to the encoder, we also connect another switch to show how to add additional buttons.
 
 As with many IoAbstraction features it relies on TaskManagerIO library.

 For the circuit and more information:
 https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/arduino-switches-handled-as-events/
 https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
 https://www.thecoderscorner.com/ref-docs/ioabstraction/html/index.html
*/

#include<IoAbstraction.h>
#include <TaskManagerIO.h>

// Make sure the code builds.
#ifndef LED_BUILTIN
#define LED_BUILTIN 1
#endif

// The pin onto which we connected the rotary encoders switch
const pinid_t spinwheelClickPin = 7;

// The pin onto which we connected the repeat button switch
const pinid_t repeatButtonPin = 9;

// The two pins where we connected the A and B pins of the encoder, the A pin must support interrupts.
const pinid_t encoderAPin = 3;
const pinid_t encoderBPin = 4;

// the maximum (0 based) value that we want the encoder to represent.
const int maximumEncoderValue = 128;

// an LED that flashes as the encoder changes
const int ledOutputPin = LED_BUILTIN;

//
// When the encoder button is clicked, this function will be run as we registered it as a callback
//
void onSpinwheelClicked(pinid_t pin, bool heldDown) {
    Serial.print("Button pressed ");
    Serial.print(pin);
    Serial.println(heldDown ? " Held" : " Pressed");
}

//
// When the repeat button is pressed, this function will be repeatedly called. It's also a callback
//
void onRepeatButtonClicked(pinid_t /*pin*/, bool /*heldDown*/) {
    Serial.println("Repeat button pressed");
}

//
// Each time the encoder value changes, this function runs, as we registered it as a callback
//
void onEncoderChange(int newValue) {
    Serial.print("Encoder change ");
    Serial.println(newValue);

    // here we turn the LED on and off as the encoder moves.
    internalDigitalDevice().digitalWriteS(ledOutputPin, newValue % 2);
}

void setup() {

    Serial.begin(115200);
    Serial.println("Starting rotary encoder example");

    // We want to make the onboard LED flash, so set the pin to be output
    internalDigitalDevice().pinMode(ledOutputPin, OUTPUT);

    // our next task is to initialise swtiches, do this BEFORE doing anything else with switches.
    // We choose to initialise in poll everything (requires no interrupts), but there are other modes too:
    // (SWITCHES_NO_POLLING - interrupt only) or (SWITCHES_POLL_KEYS_ONLY - encoders on interrupt)
    switches.init(asIoRef(internalDigitalDevice()), SWITCHES_POLL_EVERYTHING, true);

    // now we add the switches, we don't want the spin-wheel button to repeat, so leave off the last parameter
    // which is the repeat interval (millis / 20 basically) Repeat button does repeat as we can see.
    switches.addSwitch(spinwheelClickPin, onSpinwheelClicked, NO_REPEAT);
    const int repeatIntervalInTicks = 25;
    switches.addSwitch(repeatButtonPin, onRepeatButtonClicked, repeatIntervalInTicks);

    // now we set up the rotary encoder, we provide the A pin, B pin, and the function that is called when changed.
    // Lastly, we can optionally tell the encoder how to accelerate: HWACCEL_SLOWER, HWACCEL_REGULAR, HWACCEL_NONE.
    // once created we give the encoder to switches, it will manage it for us. "Switches" keeps encoders in an indexed
    // array, so the first will be 0 and so on.
    const int encoderSlot = 0;
    auto hwEncoder = new HardwareRotaryEncoder(encoderAPin, encoderBPin, onEncoderChange, HWACCEL_SLOWER);
    switches.setEncoder(encoderSlot, hwEncoder);

    // here we are going to change the range of the encoder, we provide the desired value and the maximum value,
    // optionally we can also set the step size (how far each click moves) and if the value should wrap around when
    // hitting the minimum and maximum.
    const int currentValue = 100;
    const int stepSize = 1;
    const bool wrapAroundOnMinMax = true;
    hwEncoder->changePrecision(maximumEncoderValue, currentValue, wrapAroundOnMinMax, stepSize);
}

void loop() {
    taskManager.runLoop();
}
