
#include <mbed.h>
#include <TaskManager.h>
#include <IoAbstraction.h>
#include <TaskManagerIO.h>

// to be able to use IoLogging within your application add the following
BufferedSerial serPort(USBTX, USBRX);
MBedLogger LoggingPort(serPort);

IoAbstractionRef outReg = outputOnlyFromShiftRegister(PG_0, PG_1, PD_1);

uint8_t nextVal = 0;

void setup() {
    // even though shift out is always output, it is a good idea to always set pins to output as if the device changes
    // your code still works
    for(int i=SHIFT_REGISTER_OUTPUT_CUTOVER; i<(SHIFT_REGISTER_OUTPUT_CUTOVER + 7); i++) {
        ioDevicePinMode(outReg, i, OUTPUT);
    }
    taskManager.scheduleFixedRate(500, [] {
        nextVal++;
        ioDeviceDigitalWritePortS(outReg, SHIFT_REGISTER_OUTPUT_CUTOVER, nextVal);
    });
}


int main() {
    serPort.set_baud(115200);
    setup();

    while(1) {
        taskManager.runLoop();
    }
}
