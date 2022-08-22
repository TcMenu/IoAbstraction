
/**
 * @file IoLogging.h
 *
 * Some very basic logging utilities for any IoAbstraction user that log to a chosen serial interface. Turned on
 * by un-commenting the define. Should NOT be used in production.
 */

#include "IoLogging.h"

#ifdef IO_LOGGING_DEBUG

unsigned int enabledLevels = IO_LOGGING_DEFAULT_LEVEL;

void serlogHexDump(SerLoggingLevel level, const char *title, const void* data, size_t strlen) {
    if(!serLevelEnabled(level)) return;

    logTimeAndLevel(title, level);
    LoggingPort.println();

    const auto str = (const uint8_t *) data;
    for (size_t ii = 0; ii < strlen; ii++) {
        LoggingPort.print((int) str[ii], HEX);
        LoggingPort.print(((ii % 8) == 7) ? '\n' : ' ');
    };
    LoggingPort.println();
}

const char* prettyLevel(SerLoggingLevel level) {
    switch(level) {
        case SER_WARNING: return "WRN";
        case SER_ERROR: return "ERR";
        case SER_DEBUG: return "DBG";
        case SER_TCMENU_INFO: return "TCM";
        case SER_TCMENU_DEBUG: return "TCD";
        case SER_NETWORK_INFO: return "NET";
        case SER_NETWORK_DEBUG: return "NTD";
        case SER_IOA_INFO: return "IOA";
        case SER_IOA_DEBUG: return "IOD";
        case SER_USER_1: return "U01";
        case SER_USER_2: return "U02";
        case SER_USER_3: return "U03";
        case SER_USER_4: return "U04";
        default: return "???";
    }
}

#endif
