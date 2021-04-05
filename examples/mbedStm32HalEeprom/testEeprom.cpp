/**
 * This example writes to the STM32 battery backup area using HAL functions available on most STM boards. It writes
 * values to the ROM area, commits them into storage, then refreshes and loads them back. It only writes once, but
 * reads back many times, so is safe to run for longer than a few seconds.
 *
 * Note that this only works on STM32F4 and other STM boards that support battery backed RAM via HAL functions.
 * You must define IOA_ENABLE_STM32_HAL_EXTRAS in the compiler flags in order to enable this support.
 */

#include <mbed/HalStm32EepromAbstraction.h>
#include <IoLogging.h>
#include <TaskManagerIO.h>

// to be able to use IoLogging within your application add the following
BufferedSerial serPort(USBTX, USBRX);
MBedLogger LoggingPort(serPort);

// create the EEPROM
HalStm32EepromAbstraction eeprom;

// where to start within the battery backed RAM.
#define OFFSET_ROM_AREA 1024

// a couple of string buffers for reading and writing.
const char stringToWrite[] = "this is an array that needs writing to ROM";
char readBuffer[64];

bool running = true;

int main() {
    serPort.set_baud(115200);

    // before anything else, initialise the ROM.
    eeprom.initialise(OFFSET_ROM_AREA);

    // we can always check for errors, such as initialisation failure or exceeding bounds.
    serdebugF2("Error status: ", eeprom.hasErrorOccurred());

    // here we'll write this short and read it back later
    uint8_t integerValue = 42;

    // now write out some values.
    eeprom.write32(0, 0xd00db00d);
    eeprom.write16(4, 0xfade);
    eeprom.write8(6, integerValue);
    eeprom.writeArrayToRom(8, (const uint8_t*)stringToWrite, sizeof stringToWrite);

    // actually commit them into the memory
    eeprom.commit();

    // read it back from backup RAM..
    eeprom.refresh();

    // and schedule a read back
    taskManager.scheduleFixedRate(2500, [integerValue]() {
        uint8_t byteRead = eeprom.read8(6);
        uint16_t shortRead = eeprom.read16(4);
        uint32_t longRead = eeprom.read32(0);
        serdebugF3("byte read  (read-back, original)", byteRead, integerValue);
        serdebugF2("short read (read-back", shortRead);
        serdebugF2("long read  (read-back, original)", longRead);
    });

    while(running) {
        taskManager.runLoop();
    }
}