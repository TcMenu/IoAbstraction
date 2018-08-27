/**
 * This example shows how to use an MCP23017 with interrupt support, we connect some
 * LEDS to port B and set them to cycle based on the state of the button connected to
 * a pin on port A. We use this libraries switches support  to check port A using interrupts
 * for changes.
 * 
 * It would actually be easier to use the inbuilt port support but this is for examples sake.
 * See directPortAccess for more about using ports.
 */

#include <IoAbstraction.h>
#include <IoAbstractionWire.h>

// the pin where the switch is connected
int switchPin = 6;
// the pin where the LEDs start (I've connected 4 )
int ledPinStart = 8;
int arduinoInterruptPin = 2;

IoAbstractionRef io23017 = ioFrom23017(0x20/*, ACTIVE_LOW_OPEN, arduinoInterruptPin*/);

byte currentCount = 0;

//
// this function is called by switches whenever the button is pressed.
//
void onKeyPressed(uint8_t key, bool held) {
    currentCount++; // bump the count.
    
    // now we draw the binary representation of the first 4 bits onto LEDs.
    // I've got the LEDs from 5V to the output pin, so logic is inverted.
    ioDeviceDigitalWrite(io23017, ledPinStart, (currentCount & 1) == 0);
    ioDeviceDigitalWrite(io23017, ledPinStart + 1, (currentCount & 2) == 0);
    ioDeviceDigitalWrite(io23017, ledPinStart + 2, (currentCount & 4) == 0);
    ioDeviceDigitalWrite(io23017, ledPinStart + 3, (currentCount & 8) == 0);

    // and log the current state to serial.
    Serial.print("Key pressed "); Serial.print(key);
    Serial.print(" current count "); Serial.println(currentCount);
}

void setup() {
    // startup wire and serial.
    Wire.begin();
    Serial.begin(9600);

    Serial.println("Starting LED on 23017 example");

    ioDevicePinMode(io23017, ledPinStart, OUTPUT);
    ioDevicePinMode(io23017, ledPinStart + 1, OUTPUT);
    ioDevicePinMode(io23017, ledPinStart + 2, OUTPUT);
    ioDevicePinMode(io23017, ledPinStart + 3, INPUT_PULLUP);

    switches.initialise(io23017, true);
    switches.addSwitch(switchPin, onKeyPressed, 20); // call onKey pressed with repeat.
}

void loop() {
    // switches needs task manager to run, we must therefore call it every loop and avoid using delays,
    // instead schedule stuff to be done.
    taskManager.runLoop();
}