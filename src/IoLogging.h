#ifndef _IO_LOGGING_H_
#define _IO_LOGGING_H_

/**
 * @file IoLogging.h
 *
 * Some very basic logging utilities for any IoAbstraction user that log to a chosen serial interface. Turned on
 * by un-commenting the define. Should NOT be used in production.
 */

#include "PlatformDetermination.h"

// START user adjustable section.

// When line below commented out - no logging, when un-commented - logging. You can also define this macro in your build system
//#define IO_LOGGING_DEBUG

// END user adjustable section.

#ifdef IO_LOGGING_DEBUG


#ifdef IOA_USE_MBED

#include "PrintCompat.h"
#include <FileHandle.h>
//
// On mbed you create an instance of this class called LoggingPort in your main class.
// see the mbed example.
//
class MBedLogger : public Print {
private:
    FileHandle& serial;
public:
    MBedLogger(FileHandle& serialName) : serial(serialName) {}

    size_t write(uint8_t ch) override {
        serial.write(&ch, 1);
        return 1;
    }

    size_t write(const char* sz) override {
        auto len = strlen(sz);
        serial.write(sz, len);
        return len;
    }
};
extern MBedLogger LoggingPort;
// a couple of definitions here to avoid including headers, F() macro not needed on mbed
unsigned long millis();
#define F(x) x
#else

// Arduino:
// You can change the logging serial port by defining LoggingPort to your chosen serial port.
#ifndef LoggingPort
#define LoggingPort Serial
#endif
#endif

#define logTime(title) LoggingPort.print(millis());LoggingPort.print(':');LoggingPort.print(title)
#define serdebugF(x) logTime(F(x));LoggingPort.println();
#define serdebugF2(x1, x2) logTime(F(x1)); LoggingPort.print(x2);LoggingPort.println();
#define serdebugF3(x1, x2, x3) logTime(F(x1)); LoggingPort.print(x2); LoggingPort.print(' '); LoggingPort.print(x3);LoggingPort.println();
#define serdebugF4(x1, x2, x3, x4) logTime(F(x1)); LoggingPort.print(x2); LoggingPort.print(' '); LoggingPort.print(x3); LoggingPort.print(' '); LoggingPort.print(x4);LoggingPort.println();
#define serdebugFHex(x1, x2) logTime(F(x1)); LoggingPort.print(x2, HEX);LoggingPort.println();
#define serdebugFHex2(x1, x2, x3) logTime(F(x1)); LoggingPort.print(x2, HEX); LoggingPort.print(','); LoggingPort.print(x3, HEX);LoggingPort.println();
#define serdebug(x) logTime(x);LoggingPort.println();
#define serdebug2(x1, x2) logTime(x1); LoggingPort.print(x2);LoggingPort.println();
#define serdebug3(x1, x2, x3) logTime(x1); LoggingPort.print(x2); LoggingPort.print(' '); LoggingPort.print(x3);LoggingPort.println();
#define serdebugHex(x1, x2) logTime(x1); LoggingPort.print(x2, HEX);LoggingPort.println();
inline void serdebugHexDump(const char *title, const void* data, size_t strlen) {
    logTime(title);
    LoggingPort.println();

    const auto str = (const uint8_t *) data;
    for (size_t ii = 0; ii < strlen; ii++) {
        LoggingPort.print((int) str[ii], HEX);
        LoggingPort.print(((ii % 8) == 7) ? '\n' : ' ');
    };
    LoggingPort.println();
}
#else
// all loging to no operations (commenting out the above define of IO_LOGGING_DEBUG to remove in production builds).
#define serdebugF(x) 
#define serdebugF2(x, y) 
#define serdebugF3(x, y, z) 
#define serdebugF4(a, b, c, d) 
#define serdebugFHex(x, y) 
#define serdebugFHex2(x, y, z) 
#define serdebug(x) 
#define serdebug2(x, y) 
#define serdebug3(x, y, z) 
#define serdebugHex(x, y) 
#define serdebugHexDump(x, str, strlen)
#endif // Logging enabled

#endif // header include
