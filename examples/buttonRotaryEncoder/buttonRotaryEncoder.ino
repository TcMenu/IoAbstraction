/*
 This sketch gives an example of using a rotary encoder along with it's inbuilt push button with
 the switch input facilities. In addition to the encoder, we also connect another switch that
 will be used to 
 
 Switch input is designed to work with the task manager class which
 makes scheduling tasks trivial.

 Circuit / detail: https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/arduino-switches-handled-as-events/

*/

#include<IoAbstraction.h>
#include <TaskManagerIO.h>

// The pin onto which we connected the rotary encoders switch
const int spinwheelClickPin = 18;

// The pin onto which we connected the repeat button switch
const int repeatButtonPin = 3;

// The two pins where we connected the A and B pins of the encoder. I recomend you dont change these
// as the pin must support interrupts.
const int encoderAPin = 14;
const int encoderBPin = 17;

// the maximum (0 based) value that we want the encoder to represent.
const int maximumEncoderValue = 128;

// an LED that flashes as the encoder changes
const int ledOutputPin = 26;

auto boardIo = internalDigitalIo();

//
// When the spinwheel is clicked, this function will be run as we registered it as a callback
//
void onSpinwheelClicked(pinid_t pin, bool heldDown) {
  Serial.print("Button pressed ");
  Serial.println(heldDown ? "Held" : "Pressed");
}

//
// When the repeat button is pressed, this function will be repeatedly called. It's also a callback
//
void onRepeatButtonClicked(pinid_t pin, bool heldDown) {
  Serial.println("Repeat button pressed");
}

//
// Each time the encoder value changes, this function runs, as we registered it as a callback
//
void onEncoderChange(int newValue) {
  Serial.print("Encoder change ");
  Serial.println(newValue);

  // here we turn the led on and off as the encoder moves.
  ioDeviceDigitalWriteS(boardIo, ledOutputPin, newValue % 2);
}

void setup() {

  Serial.begin(115200);

  // here we initialise as output the output pin we'll use
  ioDevicePinMode(boardIo, ledOutputPin, OUTPUT);

  // First we set up the switches library, giving it the task manager and tell it to use arduino pins
  // We could also of chosen IO through an i2c device that supports interrupts.
  // If you want to use PULL DOWN instead of PULL UP logic, change the true to false below.
  switches.initialise(ioUsingArduino(), true);

  // now we add the switches, we dont want the spinwheel button to repeat, so leave off the last parameter
  // which is the repeat interval (millis / 20 basically) Repeat button does repeat as we can see.
  switches.addSwitch(spinwheelClickPin, onSpinwheelClicked);
  switches.addSwitch(repeatButtonPin, onRepeatButtonClicked, 25);

  // now we set up the rotary encoder, first we give the A pin and the B pin.
  // we give the encoder a max value of 128, always minumum of 0.
  setupRotaryEncoderWithInterrupt(encoderAPin, encoderBPin, onEncoderChange);
  switches.changeEncoderPrecision(maximumEncoderValue, 100);
}

void loop() {
  taskManager.runLoop();  
}
