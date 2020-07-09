/**
 * This is a simple example of using the Joystick rotary encoder. It assumes you have a potentiometer
 * based analog joystick connected to an analog input port.
 * 
 * Shows the value of a rotary encoder over the serial port. Wiring, wire the joystick as per instructions
 * and take note of the analog input pin you've used. 
 */

#include <IoAbstraction.h>
#include <JoystickSwitchInput.h>

#define ANALOG_INPUT_PIN A1

// we need to create an analog device that the joystick encoder will use to get readings.
// In this case on arduino analog pins.
ArduinoAnalogDevice analogDevice;

// and we must tell switches where the buttons are located, in this case on arduino pins.
IoAbstractionRef arduinoPins = ioUsingArduino();

void onEncoderChange(int newValue) {
    Serial.print("New joystick value: ");
    Serial.print(newValue);
    Serial.print(", analog in ");
    Serial.println(analogDevice.getCurrentValue(ANALOG_INPUT_PIN));
}

void setup() {
    while(!Serial);
    Serial.begin(115200);

    Serial.println("Starting joystick rotary encoder example");

    // first initialise switches using pull up switch logic.
    switches.initialise(arduinoPins, true);

    // now register the joystick
    setupAnalogJoystickEncoder(&analogDevice, ANALOG_INPUT_PIN, onEncoderChange);

    // now set the range to 500 and current value to 250
    switches.changeEncoderPrecision(500, 250);

    // and that's it, task manager and switches does the register
    Serial.println("Started joystick example");
}

void loop() {
    // we must always call this every loop cycle and as frequently as possible
    taskManager.runLoop();
}
