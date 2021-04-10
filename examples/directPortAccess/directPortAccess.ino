/**
 * A sketch that shows how to directly access a port on both an MCP23017 IoExpander
 * along with arduino pins, using the MultiIoAbstraction to combine them into one.
 * The multi IO abstraction defaults to Arduino pins using the range 0..99 and the
 * first expander (our 23017) will appear at pin 100 onwards.
 * 
 * IMPORTANT NOTE: This example assumes a MEGA2560 or other device that has a port
 * mapped to pins 42 to 49 and will need modification to work with any other board.
 * 
 * If using the Arduino direct pins, be very careful which port you choose, as some
 * pins have special functions, and won't take kindly to being stamped on.. When using
 * with i2c or shift registers there is no risk of this whatsoever.
 * 
 * To access a port, we simply provide a pin that is within the ports range.
 */

#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <TaskManagerIO.h>

// we are going to use a MultiIoAbstraction to let us have both arduino pins and an MCP23017 i2c
// in one abstraction. Arduino pins will go to 99, expander from 100..109.
MultiIoAbstraction allDevices;
uint8_t ledState = 0xaa;

// We'll use port that starts at pin 42 on the Mega, change for your board.
#define PORT_L_OFFSET 42

// we will program port B on the IoExpander
#define EXPANDER_OFFSET 108

// we need to make sure we reset the 23017 properly on restart..
#define RESET_23017_PIN 32

void setup() {
    Wire.begin();
    Serial.begin(115200);

    Serial.println("Multi IO expander example");

    // this is optional, in a real world system you could probably just connect the
    // reset pin of the device to Vcc, but when prototyping you'll want a reset
    // on every restart.
    pinMode(RESET_23017_PIN, OUTPUT);
    digitalWrite(RESET_23017_PIN, LOW);
    delayMicroseconds(100);
    digitalWrite(RESET_23017_PIN, HIGH);

    Serial.println("Finished reset, adding the 23017");

    // when using multiIO the pins are arranged with each device owning some of the pins, in this case
    // Arduino: 0..99
    // 23017: 100..119
    allDevices.addIoExpander(ioFrom23017(0x20), 120);

    Serial.println("Setting up IoExpander inputs and outputs");

    // in this case we want all the pins as output, so we set all the pins we are
    // going to use first to the right state.
    for(int i=0; i<8; i++) {
        ioDevicePinMode(&allDevices, EXPANDER_OFFSET + i, OUTPUT);
        ioDevicePinMode(&allDevices, PORT_L_OFFSET + i, OUTPUT);
    }
}

void loop() {
    Serial.println("In loop, writing to ports and doing sync");

    // and now we write to the port directly. Use caution before writing directly
    // to ports, read the arduino guides on this subject first.
    allDevices.writePort(PORT_L_OFFSET, ledState);
    allDevices.writePort(EXPANDER_OFFSET, ledState);
    ioDeviceSync(&allDevices); // make sure any serial devices are updated.

    Serial.print("This loop ledState was "); Serial.println(ledState, HEX);

    ledState = (ledState == 0xaa) ? 0x55 : 0xaa;
    delay(1000);
}
