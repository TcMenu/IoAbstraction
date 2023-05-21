/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef IOABSTRACTION_SPIHELPER_H
#define IOABSTRACTION_SPIHELPER_H

#include "../PlatformDetermination.h"
#include "../IoAbstraction.h"

#ifdef IOA_USE_ARDUINO
#include <SPI.h>
class SPIWithSettings {
private:
    HardwareSPI* spiBus;
    SPISettings settings;
    pinid_t csPin = 0;
public:
    SPIWithSettings(HardwareSPI* bus, pinid_t cs) : spiBus(bus), csPin(cs) {
        internalDigitalDevice().pinMode(cs, OUTPUT);
        internalDigitalDevice().digitalWrite(cs, HIGH);
    }

    SPIWithSettings(HardwareSPI* bus, pinid_t cs, const SPISettings& settings) : spiBus(bus), settings(settings), csPin(cs) {
        internalDigitalDevice().pinMode(cs, OUTPUT);
        internalDigitalDevice().digitalWrite(cs, HIGH);
    }

    void transferSPI(uint8_t* rdwr, size_t len) {
        internalDigitalDevice().digitalWrite(csPin, LOW);
        spiBus->beginTransaction(settings);
        spiBus->transfer(rdwr, len);
        spiBus->endTransaction();
        internalDigitalDevice().digitalWrite(csPin, HIGH);
    }
};
#else
#error "Not implemented yet for mbed"
#endif

#endif //IOABSTRACTION_SPIHELPER_H
