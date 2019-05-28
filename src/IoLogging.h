#ifndef _IO_LOGGING_H_
#define _IO_LOGGING_H_

/**
 * @file IoLogging.h
 *
 * Some very basic logging utilities for any IoAbstraction user that log to a chosen serial interface. Turned on  by un-commenting
 * the IO_LOGGING_ON define.
 */

// START user adjustable section.

// When line below commented out - no logging, when un-commented - logging.
#define IO_LOGGING_DEBUG

// END user adjustable section.

#ifdef IO_LOGGING_DEBUG

// change to log elsewhere than Serial.
#define LoggingPort Serial

#define logTime(title) LoggingPort.print(millis());LoggingPort.print(':');LoggingPort.print(title)
#define serdebugF(x) logTime(F(x));LoggingPort.println();
#define serdebugF2(x1, x2) logTime(F(x1)); LoggingPort.println(x2);
#define serdebugF3(x1, x2, x3) logTime(F(x1)); LoggingPort.print(x2); LoggingPort.print(' '); LoggingPort.println(x3);
#define serdebugF4(x1, x2, x3, x4) logTime(F(x1)); LoggingPort.print(x2); LoggingPort.print(' '); LoggingPort.print(x3); LoggingPort.print(' '); LoggingPort.println(x4);
#define serdebugFHex(x1, x2) logTime(F(x1)); LoggingPort.println(x2, HEX);
#define serdebugFHex2(x1, x2, x3) logTime(F(x1)); LoggingPort.print(x2, HEX); LoggingPort.print(','); LoggingPort.println(x3, HEX);
#define serdebug(x) logTime(x);LoggingPort.println();
#define serdebug2(x1, x2) logTime(x1); LoggingPort.println(x2);
#define serdebug3(x1, x2, x3) logTime(x1); LoggingPort.print(x2); LoggingPort.print(' '); LoggingPort.println(x3);
#define serdebugHex(x1, x2) logTime(x1); LoggingPort.println(x2, HEX);
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
#endif // Logging enabled

#endif // header include
