/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef IOABSTRACTION_SPIHELPER_H
#define IOABSTRACTION_SPIHELPER_H

#include "../PlatformDetermination.h"
#include "../IoAbstraction.h"

#define SPI_TEN_MHZ (10 * 1000000)

#ifdef IOA_USE_ARDUINO
#include <SPI.h>
#ifdef ESP32
#define HardwareSPI SPIClass
#endif
class SPIWithSettings {
private:
    HardwareSPI* spiBus;
    SPISettings settings;
    pinid_t csPin = 0;
    bool initializedYet = false;
public:
    SPIWithSettings(HardwareSPI* bus, pinid_t cs) : spiBus(bus), csPin(cs) {}
    SPIWithSettings(HardwareSPI* bus, pinid_t cs, const SPISettings& settings) : spiBus(bus), settings(settings), csPin(cs) {}
    SPIWithSettings(const SPIWithSettings&) = default;
    SPIWithSettings& operator=(const SPIWithSettings&)=default;

    void init() {
        internalDigitalDevice().pinMode(csPin, OUTPUT);
        internalDigitalDevice().digitalWrite(csPin, HIGH);
    }

    bool transferSPI(uint8_t* rdwr, size_t len) {
        if(!initializedYet) {
            init();
        }
        internalDigitalDevice().digitalWrite(csPin, LOW);
        spiBus->beginTransaction(settings);
        spiBus->transfer(rdwr, len);
        spiBus->endTransaction();
        internalDigitalDevice().digitalWrite(csPin, HIGH);
        return true;
    }
};
#elif BUILD_FOR_PICO_CMAKE
#include "../pico/i2cWrapper.h"
#elif BUILD_FOR_STM32CUBE_CMAKE
#include "../stmCube/CubeI2cWrapper.h"
#elif __MBED__
#include "../mbed/mbedSpi.h"
#elif BUILD_FOR_NATIVE_PLATFORM
#include "../testing/TestWire.h"
#else
#error "Not implemented yet for chosen platform"
#endif

#endif //IOABSTRACTION_SPIHELPER_H
