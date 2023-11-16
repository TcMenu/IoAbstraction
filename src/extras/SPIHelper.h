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
        internalDigitalDevice().pinMode(cs, OUTPUT);
        internalDigitalDevice().digitalWrite(cs, HIGH);
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
#include "hardware/spi.h"
#define TC_SPI_WRITE_AVAILABLE
class SPIWithSettings {
private:
    spi_inst_t* spiBus;
    uint32_t speed;
    pinid_t csPin = 0;
    bool initializedYet = false;
public:
    SPIWithSettings(spi_inst_t* bus, pinid_t cs) : spiBus(bus), csPin(cs), speed(10000000) {}
    SPIWithSettings(spi_inst_t* bus, pinid_t cs, uint32_t speed) : spiBus(bus), speed(speed), csPin(cs) {}
    SPIWithSettings(const SPIWithSettings&) = default;
    SPIWithSettings& operator=(const SPIWithSettings&)=default;

    void init() {
        internalDigitalDevice().pinMode(csPin, OUTPUT);
        internalDigitalDevice().digitalWrite(csPin, HIGH);
        initializedYet=true;
    }

    void waitABit() {
        asm volatile("nop \n nop \n nop");
    }

    void waitAndActiveCS() {
        if(!initializedYet) {
            init();
        }
        int retries = 50;
        while(spi_is_busy(spiBus) && retries > 0) {
            retries--;
            serlogF2(SER_IOA_DEBUG, "SPI busy retries=", retries);
        }

        internalDigitalDevice().digitalWrite(csPin, LOW);
        waitABit();
    }

    void waitAndDeactivateCS() {
        waitABit();
        internalDigitalDevice().digitalWrite(csPin, HIGH);
        waitABit();
    }

    bool write(const uint8_t* data, size_t size) {
        waitAndActiveCS();
        int written = spi_write_blocking(spiBus, data, size);
        waitAndDeactivateCS();
        return written == size;
    }

    bool transferSPI(uint8_t* rdwr, size_t len) {
        waitAndActiveCS();
        int written = spi_write_read_blocking(spiBus, rdwr, rdwr, len);
        waitAndDeactivateCS();
        return written == len;
    }
};
#else
#error "Not implemented yet for mbed"
#endif

#endif //IOABSTRACTION_SPIHELPER_H
