
#ifdef BUILD_FOR_PICO_CMAKE

#include "i2cWrapper.h"
#include "PlatformDeterminationWire.h"

// on pico i2c is set up by the user before first use
PicoI2cWrapper defaultWireType;
WireType defaultWireTypePtr = &defaultWireType;

bool PicoI2cWrapper::wireRead(uint8_t addr, uint8_t *dst, size_t len) {
    auto amtAvail = i2c_get_read_available(nativeI2c);
    if(amtAvail < len) return false;
    return i2c_read_blocking(nativeI2c, addr, dst, len, false);
}

bool PicoI2cWrapper::wireWrite(uint8_t addr, const uint8_t *src, size_t len, int retries, bool sendStop) {
    int tries = 0;
    while(i2c_write_blocking(nativeI2c, addr, src, len, !sendStop) <= 0) {
        if(tries > retries) return false;
        taskManager.yieldForMicros(50);
        tries++;
    }
    return true;
}

void ioaWireBegin(i2c_inst_t * i2c) {
    defaultWireTypePtr->init(i2c);
}

bool ioaWireRead(WireType wire, int address, uint8_t* buffer, size_t len) {
    if(wire == nullptr || !wire->isValid()) return false;
    return wire->wireRead(address, buffer, len);
}

bool ioaWireWriteWithRetry(WireType wire, int address, const uint8_t* buffer, size_t len, int retriesAllowed, bool sendStop) {
    if(wire == nullptr || !wire->isValid()) return false;
    return wire->wireWrite(address, buffer, len, retriesAllowed, sendStop);
}

#endif