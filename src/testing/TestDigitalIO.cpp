#ifdef BUILD_FOR_NATIVE_PLATFORM

#include "../MockIoAbstraction.h"

BasicIoAbstraction internalIoAbstraction;

IoAbstractionRef internalDigitalIo() {
    return &internalIoAbstraction;
}

void BasicIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
}

void BasicIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
}

uint8_t BasicIoAbstraction::readValue(pinid_t pin) {
    return 0;
}

void BasicIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
}

void BasicIoAbstraction::writePort(pinid_t port, uint8_t portVal) {
}

uint8_t BasicIoAbstraction::readPort(pinid_t port) {
    return 0;
}

#endif
