

#include "PlatformDetermination.h"
#include "BasicIoAbstraction.h"

#if defined(IOA_USE_ARDUINO) && !defined(ESP32)

void BasicIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    pinMode(pin, mode);
}

void BasicIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
    digitalWrite(pin, value);
}

uint8_t BasicIoAbstraction::readValue(pinid_t pin) {
    return digitalRead(pin);
}

void BasicIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
    pintype_t intPin = digitalPinToInterrupt(pin);
    internalHandleInterrupt(intPin, interruptHandler, mode);
}

void BasicIoAbstraction::writePort(pinid_t port, uint8_t portVal) {
	*portOutputRegister(digitalPinToPort(port)) = portVal;
}

uint8_t BasicIoAbstraction::readPort(pinid_t port) {
	return *portInputRegister(digitalPinToPort(port));
}

IoAbstractionRef arduinoAbstraction = NULL;
IoAbstractionRef ioUsingArduino() {
    if (arduinoAbstraction == NULL) {
        arduinoAbstraction = new BasicIoAbstraction();
    }
    return arduinoAbstraction;
}

#endif
