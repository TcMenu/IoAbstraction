/**
 * A sketch that shows how to directly access a port on both an MCP23017 IoExpander
 * along with arduino pins, using the MultiIoAbstraction to combine them into one.
 * The multi IO abstraction defaults to Arduino pins using the range 0..99 and the
 * first expander (our 23017) will appear at pin 100 onwards.
 * 
 * THIS IS AN ADVANCED FEATURE OF THE LIBRARY, USE WITH CARE. Read the arduino guides
 * on working directly with ports first. 
 * 
 * IMPORTANT NOTE: This example assumes a MEGA2560 or other device that has a port
 * mapped to pins 42 to 49 and will need modification to work with any other board.
 * 
 * To access a port, we simply provide a pin that is within the ports range.
 */

#include <IoAbstraction.h>
#include <IoAbstractionWire.h>

// we are going to use a MultiIoAbstraction to let us have both arduino pins and an MCP23017 i2c
// in one abstraction. Arduino pins will go to 99, expander from 100..109.
MultiIoAbstraction allDevices;
uint8_t ledState = 0xaa;

#define PORT_L_OFFSET 42
// we will program port B
#define EXPANDER_OFFSET 108

void setup() {
    Wire.begin();
    Serial.begin(9600);

    Serial.println("Multi IO expander example");

    // when using multiIO the pins are arranged with each device owning some of the pins, in this case
    // Arduino: 0..99
    // 23017: 100..119
    allDevices.addIoExpander(ioFrom23017(0x20), 120);

    // in this case we want all the pins as output, so we set all the pins we are
    // going to use first to the right state.
    for(int i=0; i<8; i++) {
        ioDevicePinMode(&allDevices, EXPANDER_OFFSET + i, OUTPUT);
        ioDevicePinMode(&allDevices, PORT_L_OFFSET + i, OUTPUT);
    }
}

void loop() {
    // and now we write to the port directly. Use caution before writing directly
    // to ports, read the arduino guides on this subject first.
    allDevices.writePort(PORT_L_OFFSET, ledState);
    allDevices.writePort(EXPANDER_OFFSET, ledState);
    ioDeviceSync(&allDevices); // make sure any serial devices are updated.

    Serial.print("This loop ledState was "); Serial.println(ledState, HEX);

    ledState = (ledState == 0xaa) ? 0x55 : 0xaa;
    delay(1000);
}