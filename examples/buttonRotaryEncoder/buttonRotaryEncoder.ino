/*
 This sketch gives an example of using a rotary encoder along with its inbuilt push button with
 the switch input facilities. In addition to the encoder, we also connect another switch that
 will be used to 
 
 Switch input is designed to work with the task manager class which
 makes scheduling tasks trivial.

 Circuit / detail: https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/arduino-switches-handled-as-events/

*/

#include<IoAbstraction.h>
#include <TaskManagerIO.h>

// The pin onto which we connected the rotary encoders switch
const pinid_t spinwheelClickPin = USER_BTN;

// The pin onto which we connected the repeat button switch
const pinid_t repeatButtonPin = PC9;

// The two pins where we connected the A and B pins of the encoder, the A pin must support interrupts.
const pinid_t encoderAPin = PC10;
const pinid_t encoderBPin = PC8;

// the maximum (0 based) value that we want the encoder to represent.
const int maximumEncoderValue = 128;

// an LED that flashes as the encoder changes
const int ledOutputPin = LED_BUILTIN;

auto boardIo = internalDigitalIo();

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
    ioDeviceDigitalWriteS(boardIo, ledOutputPin, newValue % 2);
}

void setup() {

    Serial.begin(115200);

    // here we initialise as output the output pin we'll use
    ioDevicePinMode(boardIo, ledOutputPin, OUTPUT);

    // First we set up the switches library, giving it the task manager and tell it to use arduino pins
    // We could also of chosen IO through an i2c device that supports interrupts.
    // If you want to use PULL DOWN instead of PULL UP logic, change the true to false below.
    switches.initialise(boardIo, true);

    // now we add the switches, we don't want the spin-wheel button to repeat, so leave off the last parameter
    // which is the repeat interval (millis / 20 basically) Repeat button does repeat as we can see.
    switches.addSwitch(spinwheelClickPin, onSpinwheelClicked, NO_REPEAT, true);
    switches.addSwitch(repeatButtonPin, onRepeatButtonClicked, 25);

    // now we set up the rotary encoder, first we give the A pin and the B pin.
    // we give the encoder a max value of 128, always minimum of 0.
    auto hwEncoder = new HardwareRotaryEncoder(encoderAPin, encoderBPin, onEncoderChange);
    switches.setEncoder(0, hwEncoder);
    hwEncoder->changePrecision(maximumEncoderValue, 100);
}

void loop() {
    taskManager.runLoop();
}
