/*
 This sketch gives an example of using a rotary encoder along with it's inbuilt push button with
 the switch input facilities. In addition to the encoder, we also connect another switch that
 will be used to 
 
 Switch input is designed to work with the task manager class which
 makes scheduling tasks trivial.

 
*/

#include<BasicIoAbstraction.h>
#include<Wire.h>

// We need a task manager and a switch conroller for this. The task manager takes care of scheduling
// and the switch input takes care of debouncing, doing callbacks and decoding.
TaskManager taskManager;
SwitchInput switches;

// The pin onto which we connected the rotary encoders switch
const int spinwheelClickPin = 24;

// The pin onto which we connected the repeat button switch
const int repeatButtonPin = 25;

// The two pins where we connected the A and B pins of the encoder. I recomend you dont change these
// as the pin must support interrupts.
const int encoderAPin = 2;
const int encoderBPin = 3;

// the maximum (0 based) value that we want the encoder to represent.
const int maximumEncoderValue = 128;

//
// When the spinwheel is clicked, this function will be run as we registered it as a callback
//
void onSpinwheelClicked(uint8_t pin, bool heldDown) {
  Serial.print("Button pressed ");
  Serial.println(heldDown ? "Held" : "Pressed");
}

//
// When the repeat button is pressed, this function will be repeatedly called. It's also a callback
//
void onRepeatButtonClicked(uint8_t pin, bool heldDown) {
  Serial.println("Repeat button pressed");
}

//
// Each time the encoder value changes, this function runs, as we registered it as a callback
//
void onEncoderChange(int newValue) {
  Serial.print("Encoder change ");
  Serial.println(newValue);
}

//
// We register an interrupt handler to manage changes to the rotary encoder.
//
void encoderInterrupt(uint8_t pin) {
  // switch input provides a ready written function to handle rotary encoder interrupts.
  onEncoderInterrupt();
}

void setup() {

  Serial.begin(9600);

  // First we set up the switches library, giving it the task manager and tell it to use arduino pins
  // We could also of chosen IO through an i2c device that supports interrupts.
  switches.initialise(taskManager, ioUsingArduino());

  // now we add the switches, we dont want the spinwheel button to repeat, so leave off the last parameter
  // which is the repeat interval (millis / 20 basically) Repeat button does repeat as we can see.
  switches.addSwitch(spinwheelClickPin, onSpinwheelClicked);
  switches.addSwitch(repeatButtonPin, onRepeatButtonClicked, 25);

  // now we set up the rotary encoder, first we give the A pin and the B pin.
  // the precision works as a division, 1 results in 65355 possible values, 2 is 32678, 8 would be 256, etc.
  // last provide a function that is called when there is a material change in the encoder.
  switches.initialiseEncoder(encoderAPin, encoderBPin, onEncoderChange);
  switches.changeEncoderPrecision(maximumEncoderValue, 100);

  // rotary encoders need to be checked constantly, the only realistic way to use on is with an interrupt
  // handler to pick up changes, fortunately, task manager makes this trivial. We provide our callback
  // and ask taskmanager to interrupt on a change in pinA.
  taskManager.setInterruptCallback(encoderInterrupt);
  taskManager.addInterrupt(encoderAPin, CHANGE);
}

void loop() {
  taskManager.runLoop();  
}
