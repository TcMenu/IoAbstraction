#ifndef _IO_LOGGING_H_
#define _IO_LOGGING_H_

/**
 * @file IoLogging.h
 *
 * Some very basic logging utilities for any IoAbstraction user that log to a chosen serial interface. Turned on  by un-commenting
 * the IO_LOGGING_ON define. Should NOT be used in production.
 */

// START user adjustable section.

// When line below commented out - no logging, when un-commented - logging.
//#define IO_LOGGING_DEBUG

// END user adjustable section.

#ifdef IO_LOGGING_DEBUG


#ifdef __MBED__
#define DEC 1
#define HEX 2

//
// On mbed you create an instance of this class called LoggingPort in your main class.
// see the mbed example.
//
class MBedLogger {
private:
    Stream& serial;
public:
    MBedLogger(Stream& serialName) : serial(serialName) {}
    void print(const char* sz) { serial.puts(sz);}
    void print(char ch) { serial.putc(ch); }
    void print(bool b) { serial.puts(b ? "true" : "false"); }
    void print(int val, int mode = DEC) { char sz[20]; itoa(val, sz, mode == DEC ? 10 : 16); serial.puts(sz);}
    void print(unsigned int val, int mode = DEC) { char sz[20]; itoa(val, sz, mode == DEC ? 10 : 16); serial.puts(sz);}
    void print(unsigned long val, int mode = DEC) { char sz[20]; itoa(val, sz, mode == DEC ? 10 : 16); serial.puts(sz);}
    void print(long val, int mode = DEC) { char sz[20]; itoa(val, sz, mode == DEC ? 10 : 16); serial.puts(sz);}
    void print(double dbl) {
        char sz[20];
        int whole = int(dbl);
        int fract = int((dbl - double(whole)) * 1000.0);
        itoa((int)abs(dbl), sz, 10);
        serial.puts(sz);
        serial.putc('.');
        itoa((int)abs(fract), sz, 10);
        if(dbl < 0) serial.putc('-');
        serial.puts(sz);
    }
    void println() {
        serial.putc('\r'); serial.putc('\n');
    }
};
extern MBedLogger LoggingPort;
unsigned long millis(); // from BasicIoAbstraction.cpp to avoid including here.
#define F(x) x
#else

// Arduino:
// You can change the logging serial port by defining LoggingPort to your chosen serial port.
#define LoggingPort Serial

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
#define serdebugHexDump(x, str, strlen) logTime(x); for(int ii=0;ii<strlen;ii++) { Serial.print((int)str[ii], HEX); Serial.print(' '); }; Serial.println();
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
