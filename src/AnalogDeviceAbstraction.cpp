/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "AnalogDeviceAbstraction.h"

#ifdef IOA_USE_MBED
MBedAnalogDevice* MBedAnalogDevice::theInstance;
AnalogDevice* internalAnalogIo() {
    if(MBedAnalogDevice::theInstance == nullptr) MBedAnalogDevice::theInstance = new MBedAnalogDevice();
    return MBedAnalogDevice::theInstance;
}
#else
ArduinoAnalogDevice* ArduinoAnalogDevice::theInstance;
AnalogDevice* internalAnalogIo() {
    if(ArduinoAnalogDevice::theInstance == nullptr) ArduinoAnalogDevice::theInstance = new ArduinoAnalogDevice();
    return ArduinoAnalogDevice::theInstance;
}
#endif
