/** 
 * Advanced feature of library.
 * 
 * This example shows how to use the very simple logging that's built into IoAbstraction.
 * To enable logging open IoLogging.h in the IoAbstraction directory and uncomment
 * #define IO_LOGGING_DEBUG
 * 
 * This logging is only complied in when the above define is set, if it is not set then
 * the logging is completely removed.
 */

#include <TaskManagerIO.h>
#include <IoAbstraction.h>

char sz[] = {"hello world"};

void setup() {
    while(!Serial);
    Serial.begin(115200);

    // write a string entry that is applied with F(..) so in progmem on AVR
    // with an integer second value.
    serdebugF2("In setup function - A0=", analogRead(A0));

    taskManager.scheduleFixedRate(10, [] {
        // write values to log in HEX - first parameter is wrapped in F(..) using the F variant
        serdebugFHex2("Two Values in hex: ", 0xFADE, 0xFACE);
        serdebugFHex("One Values in hex: ", 0xFADE);
        serdebugF2("Int value: ", 109298384L);
        serdebugF2("Bool value: ", true)

        // the F variant always tries to use F(..) to save ram on the first parameter on AVR
        serdebugF("String in flash");
        
        // this version does not use F(..)
        serdebug(sz); 
    }, TIME_SECONDS);
}

void loop() {
    taskManager.runLoop();
}