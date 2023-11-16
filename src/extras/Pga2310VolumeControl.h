/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */


#ifndef IOABSTRACTION_PGA2310VOLUMECONTROL_H
#define IOABSTRACTION_PGA2310VOLUMECONTROL_H

#include "../PlatformDetermination.h"
#include "../AnalogDeviceAbstraction.h"
#include "SPIHelper.h"

/**
 * @file Pga2310VolumeControl.h
 * @brief An IoAbstraction implementation for the PGA2310 volume control that uses the standard analog device interface
 * therefore providing a consistent API. The left channel is 0 and the right channel is 1, the volume level can be
 * provided as either a float between 0 and 1, or a direct value between 0 and 255. The volume control is actually
 * between -95.5dB and +31.5dB in half steps.
 *
 * This class is in the extras package, it means it is not part of the core of IoAbstraction.
 */
class Pga2310VolumeControl : public AnalogDevice {
private:
    uint8_t leftCache = 0;
    uint8_t rightCache = 0;
    SPIWithSettings& spiBus;
public:
    explicit Pga2310VolumeControl(SPIWithSettings& spi) : spiBus(spi) {}
    int getMaximumRange(AnalogDirection direction, pinid_t pin) override { return 255; }
    int getBitDepth(AnalogDirection direction, pinid_t pin) override { return 8; }
    void initPin(pinid_t pin, AnalogDirection direction) override {
        spiBus.init();
    }
    unsigned int getCurrentValue(pinid_t pin) override { return pin == 0 ? leftCache : rightCache; }
    float getCurrentFloat(pinid_t pin) override { return float(getCurrentValue(pin)) / 255.0F; }
    void setCurrentValue(pinid_t pin, unsigned int newValue) override {
        if(pin == 0) {
            leftCache = newValue;
        } else {
            rightCache = newValue;
        }
        uint8_t dataToWrite[2] = { rightCache, leftCache };
        spiBus.transferSPI(dataToWrite, 2);
    }
    void setCurrentFloat(pinid_t pin, float newValue) override { setCurrentValue(pin, (unsigned int)(newValue * 255.0F)); }
};


#endif //IOABSTRACTION_PGA2310VOLUMECONTROL_H
