/**
 * A sketch that shows how to directly access a port on both an IoExpander (8574)
 * and also using arduino pins, notice it is the same in each case.
 * 
 * THIS IS AN ADVANCED FEATURE OF THE LIBRARY, USE WITH CARE. Read the arduino guides
 * on working directly with ports first. 
 * 
 * IMPORTANT NOTE: This example assumes a MEGA2560 and will need modification to work with
 * any other board.
 * 
 * To access a port, we simply provide a pin that is within the ports range.
 */

#include <IoAbstraction.h>
#include <IoAbstractionWire.h>

// we are going to use a MultiIoAbstraction to let us have both arduino pins and an 8574
// in one abstraction. Arduino pins will go to 99, expander from 100..109.
MultiIoAbstraction allDevices;
uint8_t ledState = 0xaa;

#define PORT_L_OFFSET 42
#define EXPANDER_OFFSET 100

void setup() {
    Wire.begin();
    
    // when using multiIO the pins are arranged with each device owning some of the pins, in this case
    // Arduino: 0..99
    // PCF8574: 100..109
    allDevices.addIoExpander(ioFrom8574(0x20), 10);

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

    ledState = (ledState == 0xaa) ? 0x55 : 0xaa;
    delay(1000);
}