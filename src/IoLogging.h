#ifndef _IO_LOGGING_H_
#define _IO_LOGGING_H_

/**
 * @file IoLogging.h
 *
 * Some very basic logging utilities for any IoAbstraction user that log to a chosen serial interface. Turned on  by un-commenting
 * the IO_LOGGING_ON define.
 */

// comment out the line below to stop logging
#define IO_LOGGING_DEBUG

// if you want to log somewhere else than Serial, change below.
#define LoggingPort Serial

#ifdef IO_LOGGING_DEBUG
#define serdebugF(x) LoggingPort.println(F(x));
#define serdebugF2(x1, x2) LoggingPort.print(F(x1)); LoggingPort.println(x2);
#define serdebugF3(x1, x2, x3) LoggingPort.print(F(x1)); LoggingPort.print(x2); LoggingPort.print(' '); LoggingPort.println(x3);
#define serdebugFHex(x1, x2) LoggingPort.print(F(x1)); LoggingPort.println(x2, HEX);
#define serdebugFHex2(x1, x2, x3) LoggingPort.print(F(x1)); LoggingPort.print(x2, HEX) LoggingPort.print(','); LoggingPort.println(x3, HEX);
#define serdebug(x) LoggingPort.println(x);
#define serdebug2(x1, x2) LoggingPort.print(x1); LoggingPort.println(x2);
#define serdebugHex(x1, x2) LoggingPort.print(x1); LoggingPort.println(x2, HEX);
#else
// all loging to no operations (commenting out the above define of IO_LOGGING_DEBUG to remove in production builds).
#define serdebugF(x) 
#define serdebugF2(x, y) 
#define serdebugF3(x, y, z) 
#define serdebugFHex(x, y) 
#define serdebug(x) 
#define serdebug2(x, y) 
#define serdebugHex(x, y) 
#endif // Logging enabled

#endif // header include
