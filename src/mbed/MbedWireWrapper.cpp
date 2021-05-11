/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <TaskManagerIO.h>
#include "PlatformDeterminationWire.h"

#ifdef IOA_USE_MBED_WIRE

// on mbed this must be set by the user.
WireType defaultWireTypePtr;

void ioaWireBegin(I2C* pI2cToUse) {
    defaultWireTypePtr = pI2cToUse;
}

void ioaWireSetSpeed(WireType i2c, long frequency) {
    i2c->frequency(frequency);
}

#if defined(DEVICE_I2C_ASYNCH) && defined(IOA_ASYNC_COMMS_ENABLED)

//
// Asynchronous I2C is currently not enabled by default, instead you need to ensure that it is enabled using the
// above flag IOA_ASYNC_COMMS_ENABLED. It is still considered experimental so turned off by default.
// It works by firstly implementing the I2C locker as a spin wait that can allow other tasks to run until the global
// comms lock can be acquired. Then the transfer function is used, and we have a simple monitor class that records
// when the operation ends, while in progress all other tasks will run as usual.
//

class MbedAsyncMonitor {
private:
    volatile bool finished = false;
    volatile bool success = false;
public:
    MbedAsyncMonitor() {
        finished = false;
    }

    bool isFinished() const { return finished; }
    bool isSuccess() const { return success; }

    void completedWithCode(int code) {
        success = (code == I2C_EVENT_TRANSFER_COMPLETE);
        finished = true;
    }
};

bool ioaWireRead(WireType pI2c, int address, uint8_t* buffer, size_t len) {
    TaskMgrLock locker(i2cLock);
    MbedAsyncMonitor monitor;
    bool ok = pI2c->transfer(address, nullptr, 0, (char*)buffer, len,
                   callback(&monitor, &MbedAsyncMonitor::completedWithCode),
                   I2C_EVENT_ALL) == 0;

    if(!ok) return false;

    int ticker = 100;
    while(!monitor.isFinished() && ticker) {
        taskManager.yieldForMicros(100);
        ticker--;
    }

    if(!monitor.isFinished()) pI2c->abort_transfer();
}

bool ioaWireWriteWithRetry(WireType pI2c, int address, const uint8_t* buffer, size_t len, int retriesAllowed, bool sendStop) {
    TaskMgrLock locker(i2cLock);
    MbedAsyncMonitor monitor;
    int tries = 0;
    
    while(tries < retriesAllowed && pI2c->transfer(address, (const char*)buffer, len, nullptr, 0,
                                                   callback(&monitor, &MbedAsyncMonitor::completedWithCode),I2C_EVENT_ALL) != 0) {
        taskManager.yieldForMicros(50);
        tries++;
    }
    
    if( tries == retriesAllowed ) return false; // did not lock the bus

    tries = 100;
    while(!monitor.isFinished() && tries) {
        taskManager.yieldForMicros(100);
        tries--;
    }

    if(!monitor.isFinished()) pI2c->abort_transfer();
}

#else 

bool ioaWireRead(WireType pI2c, int address, uint8_t* buffer, size_t len) {
    TaskMgrLock locker(i2cLock);
    return pI2c->read(address, (char*)buffer, len, false) == 0;
}

bool ioaWireWriteWithRetry(WireType pI2c, int address, const uint8_t* buffer, size_t len, int retriesAllowed, bool sendStop) {
    TaskMgrLock locker(i2cLock);
    int tries = 0;
    while(pI2c->write(address, (const char*)buffer, len, !sendStop) !=0) {
        if(tries > retriesAllowed) return false;
        taskManager.yieldForMicros(50);
        tries++;
    }
    return true;
}

#endif //DEVICE_I2C_ASYNCH

#endif
