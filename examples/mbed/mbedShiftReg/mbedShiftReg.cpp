
#include <mbed.h>
#include <TaskManager.h>
#include <IoAbstraction.h>
#include <TaskManagerIO.h>

// to be able to use IoLogging within your application the following works for most simple USBSerial implementations
// Tested with ST-Link V2.1
IOLOG_MBED_PORT_IF_NEEDED(USBTX, USBRX)

ShiftRegisterIoAbstraction outReg(ShiftRegConfig(), ShiftRegConfig(PG_0, PG_1, PD_1, 1));

uint8_t nextVal = 0;

void setup() {
    IOLOG_START_SERIAL

    // even though shift out is always output, it is a good idea to always set pins to output as if the device changes
    // your code still works
    for(int i=SHIFT_REGISTER_OUTPUT_CUTOVER; i<(SHIFT_REGISTER_OUTPUT_CUTOVER + 7); i++) {
        outReg.pinMode(i, OUTPUT);
    }
    taskManager.scheduleFixedRate(500, [] {
        nextVal++;
        outReg.writePortS(SHIFT_REGISTER_OUTPUT_CUTOVER, nextVal);
    });
}


int main() {
    setup();

    while(1) {
        taskManager.runLoop();
    }
}
