/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#if !defined(IOA_ESP32ANALOGDEVICE_H) && defined(ESP32)
#define IOA_ESP32ANALOGDEVICE_H

#include <SimpleCollections.h>
#include <AnalogDeviceAbstraction.h>

#define GPIO_INVALID -1
#define ESP32_DAC1 25
#define ESP32_DAC2 26

class EspAnalogOutputMode {
private:
    pinid_t pin;
    uint16_t pwmChannel;
    uint16_t pwmWidth;
public:
    EspAnalogOutputMode();
    EspAnalogOutputMode(const EspAnalogOutputMode& other);

    bool isDac() const { return pin == ESP32_DAC1 || pin == ESP32_DAC2; }
    pinid_t getKey() const { return pin; }
    uint16_t getPwmChannel() const { return pwmChannel; }

    void pinSetup(int pin_, int pwmChannel_);
    void write(unsigned int newVal) const;
};

class EspAnalogInputMode {
private:
    bool onAdc1;
    uint8_t adcChannelNum;
    pinid_t pin;
    int lastCached = 0;
public:
    pinid_t getKey() const { return pin; }
    EspAnalogInputMode();
    EspAnalogInputMode(const EspAnalogInputMode& other);

    void pinSetup(int pin_);
    void alterPinAttenuation(uint8_t atten) const;

    uint16_t getCurrentReading();
}

class ESP32AnalogDevice : public AnalogDevice {
private:
    BtreeList<pinid_t,EspAnalogOutputMode> gpioToPwmKey;
    BtreeList<pinid_t,EspAnalogInputMode> gpioToInputKey;
public:
    static ESP32AnalogDevice* theInstance;

    /**
	 * Initialise the ESP32 device with a given read and write bit resolution, on AVR and
	 * On ESP32 it is 12 bit input, 8 output.
	 */
	ESP32AnalogDevice();

	int getMaximumRange(AnalogDirection dir, pinid_t /*pin*/) override { return (dir == DIR_OUT) ? 255 : 4095; }

    int getBitDepth(AnalogDirection direction, pinid_t /*pin*/) override { return (direction == DIR_OUT) ? 8 : 12; }

	unsigned int getCurrentValue(pinid_t pin) override {
	    auto input = gpioToInputKey.getByKey(pin);
	    return input != nullptr ? input->getCurrentReading() : 0;
	}

	float getCurrentFloat(pinid_t pin) override {
        auto maxValue = (float)getMaximumRange(DIR_IN, pin);
        return float(getCurrentValue(pin)) / maxValue;
	}

	void setCurrentFloat(pinid_t pin, float value) override;

    void initPin(pinid_t pin, AnalogDirection direction) override;

	void setCurrentValue(pinid_t pin, unsigned int newVal) override {
	    auto output = gpioToPwmKey.getByKey(pin);
	    if(output != nullptr) output->write(newVal);
    }

    /**
     * Allows you to get access to the extended led controller parameters if it is a PWM configured pin.
     * @param pin the pin to get the configuration for
     * @return either null if not set up, or an output object
     */
    EspAnalogOutputMode* getEspOutputMode(pinid_t pin) {
	    return gpioToPwmKey.getByKey(pin);
	}

    /**
     * Allows you to get access to the extended input setup parameters if it is an input configured pin.
     * @param pin the pin to get the configuration for
     * @return either null if not set up, or an input object
     */
	EspAnalogInputMode* getEspInputMode(pinid_t pin) {
	    return gpioToInputKey.getByKey(pin);
	}
};

// for older code, mimic the name as ArduinoAnalogDevice so that code still compiles
typedef ESP32AnalogDevice ArduinoAnalogDevice;

#endif //IOA_ESP32ANALOGDEVICE_H
