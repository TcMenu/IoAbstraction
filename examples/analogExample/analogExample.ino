/**
 * This example demonstrates the analog abstraction that can use the same interface to
 * work with various analog devices such as DAC, ADC, PWM and POTs wherever they are
 * located. As more devices are added it will expand the possibilities of where This
 * common interface can be used.
 */

#include <IoAbstraction.h>
#include <AnalogDeviceAbstraction.h>

// on SAMD MKR boards this is the DAC, for Uno, MEGA change to a PWM pin
#define PWM_OR_DAC_PIN A0

// here we create the abstraction over the standard arduino analog IO capabilities
ArduinoAnalogDevice analog; // by default it assumes 10 bit read, 8 bit write

// We keep a variable that counts the output waveform
int ledCycleValue = 0;

void setup() {
    while(!Serial);
    Serial.begin(115200);

    // set up the device pin directions upfront.
    analog.initPin(A1, DIR_IN);
    analog.initPin(PWM_OR_DAC_PIN, DIR_OUT);

    // we schedule a task to run every 500 millis that reads the value from A1 and prints it output
    // along with the largest possible value
    taskManager.scheduleFixedRate(500, [] {
        Serial.print("The value on A1 is ");
        Serial.print(analog.getCurrentValue(A1));
        Serial.print("/");
        Serial.println(analog.getMaximumRange(DIR_IN, A1));
    });

    // we also create a sawtooth waveform on one of the outputs. By default we are using the DAC
    // on A0 of most MKR boards. Change to PWM for AVR boards.
    taskManager.scheduleFixedRate(25, [] {
        ledCycleValue++;
        if(ledCycleValue >= analog.getMaximumRange(DIR_OUT, PWM_OR_DAC_PIN)) {
            ledCycleValue = 0;
        }
        analog.setCurrentValue(PWM_OR_DAC_PIN, ledCycleValue);
    }, TIME_MICROS);
}

// and lastly the standard loop for use with task manager, basically does nothing but
// repeatedly call runLoop.
void loop() {
    taskManager.runLoop();
}