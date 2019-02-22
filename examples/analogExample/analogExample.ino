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
    taskManager.scheduleFixedRate(500, [] {
        Serial.print("The value on A1 is ");
        Serial.print(analog.getCurrentValue(A1));
        Serial.print("/");
        Serial.println(analog.getMaximumRange(DIR_IN, A1));
    });

    taskManager.scheduleFixedRate(25, [] {
        ledCycleValue++;
        if(ledCycleValue > analog.getMaximumRange(DIR_OUT, PWM_OR_DAC_PIN)) {
            ledCycleValue = 0;
        }
        analog.setCurrentValue(PWM_OR_DAC_PIN, ledCycleValue);
    }, TIME_MICROS);
}

void loop() {
    taskManager.runLoop();
}