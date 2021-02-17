/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _ANALOG_DEVICE_ABSTRACTION_H_
#define _ANALOG_DEVICE_ABSTRACTION_H_

#include "PlatformDetermination.h"

#include <BasicIoAbstraction.h>

/**
 * @file AnalogDeviceAbstraction.h
 * Contains a series of helper classes for dealing with analog devices, these are compatible across a wide range of
 * devices.
 */

/** 
 * an enumeration that describes direction, eg input or output for the ADC/POT/DAC. For some
 * devices only one mode will make sense.
 */
enum AnalogDirection { DIR_IN, DIR_OUT, DIR_PWM };

/**
 * Describes an analog device that has commands to both read values from and write values to
 * a device. Not all devices will support both input and output. When such a case occurs the
 * getMaximumRange would return -1 for that direction. This abstraction can support ADC, PWM
 * DAC and Potentiometer devices. On every device we support, calling internalAnalogIO gives
 * the instance for the internal analog pins.
 */
class AnalogDevice {
public:
	/**
	 * @param dir the direction required
	 * @param pin the pin for which the range is desired
	 * @return the maximum range of the analog input or output on this device for the given pin 
	 */
	virtual int getMaximumRange(AnalogDirection direction, pinid_t pin)=0;

    /**
     * @param pin the pin for which the bit depth is required.
     * @param direction the direction in which the depth is queried (DIR_IN, DIR_OUT)
     * @return the number of bits
     */
    virtual int getBitDepth(AnalogDirection direction, pinid_t pin) = 0;

	/**
	 * initialises a pin as either an input or output of analog signals. No validation to check if
	 * that pin can support input or output is performed.
	 *
	 * @param pin the pin to initialise
	 * @param direction the direction required
	 */
	virtual void initPin(pinid_t pin, AnalogDirection direction) = 0;

	/**
	 * Returns the current value on the ADC for the given pin
	 * @param pin the pin to read from
	 * @return the current value on that pin
	 */
	virtual unsigned int getCurrentValue(pinid_t pin)=0;

	/*
	 * Returns the current value on the ADC as a float between
	 * 0 and 1.
	 */
	virtual float getCurrentFloat(pinid_t pin) = 0;

	/**
	 * Sets the current value on an output capable device to a new value
	 * @param pin the pin to read from
	 * @param newValue the value to be set
	 */
	virtual void setCurrentValue(pinid_t pin, unsigned int newValue)=0;

	/**
	 * sets the current value based on a float from 0 to 1, where 0 is
	 * minimum and 1 is maximum.
	 * @param pin the pin for which to set
	 * @param newValue the new value which should be between 0 and 1.0
	 */
    virtual void setCurrentFloat(pinid_t pin, float newValue)=0;

};

#ifdef IOA_USE_MBED

class AnalogPinReference {
private:
    pinid_t pin;
    AnalogDirection direction;
    union AnalogPinReferences {
        AnalogIn* input;
#ifdef DEVICE_ANALOGOUT
        AnalogOut* out;
#endif
        PwmOut* pwm;
    } analogRef;
public:
    AnalogPinReference() {
        analogRef.input = NULL;
        pin = 0;
        direction = DIR_IN;
    }

    AnalogPinReference(const AnalogPinReference& other) {
        this->pin = other.pin;
        this->direction = other.direction;
        this->analogRef.input = other.analogRef.input;
    }

    AnalogPinReference(pinid_t pin, AnalogDirection direction) {
        this->pin = pin;
        this->direction = direction;
        switch(direction) {
            case DIR_IN:
                analogRef.input = new AnalogIn((PinName)pin);
                break;
#ifdef DEVICE_ANALOGOUT
            case DIR_OUT:
                analogRef.out = new AnalogOut((PinName)pin);
                break;
#endif
            default:
                analogRef.pwm = new PwmOut((PinName)pin);
                break;
        }
    }

    AnalogPinReferences getReferences() { return analogRef; }
    AnalogDirection getDirection() { return direction; }
    pinid_t getKey() { return pin; }
};

/**
 * Represents the mbed analog capabilities as an Analog device abstraction, allows
 * for the same code to be used between Mbed and Arduino. Note that presently the
 * value for integers is represented between 0..65535 although this may not be
 * what your hardware supports. It's better to always use the float functions when
 * you can.
 *
 * Get an instance by calling internalAnalogIO() rather than creating one.
 */
class MBedAnalogDevice : public AnalogDevice {
private:
    BtreeList<pinid_t, AnalogPinReference> devices;
public:
    static MBedAnalogDevice* theInstance;

    int getMaximumRange(AnalogDirection direction, pinid_t pin) override {
        return 0xffff;
    }

    int getBitDepth(AnalogDirection direction, pinid_t pin) override {
        return 16;
    }

    void initPin(pinid_t pin, AnalogDirection direction) override {
        if(devices.getByKey(pin) == NULL) devices.add(AnalogPinReference(pin, direction));
    }

    unsigned int getCurrentValue(pinid_t pin) override {
        auto dev = devices.getByKey(pin);
        if(dev == NULL || dev->getDirection() != DIR_IN) return 0;
        return dev->getReferences().input->read_u16();
    }

    virtual float getCurrentFloat(pinid_t pin) override {
        auto dev = devices.getByKey(pin);
        if(dev == NULL || dev->getDirection() != DIR_IN) return 0;
        return dev->getReferences().input->read();
    }

    void setCurrentValue(pinid_t pin, unsigned int newValue) override {
        auto dev = devices.getByKey(pin);
        if(dev == NULL || dev->getDirection() == DIR_IN) return;
#ifdef DEVICE_ANALOGOUT
        if(dev->getDirection() == DIR_OUT) {
            return dev->getReferences().out->write_u16(newValue);
        }
#endif
        return dev->getReferences().pwm->write(float(newValue) / 65535.0F);
    }

    void setCurrentFloat(pinid_t pin, float newValue) override {
        auto dev = devices.getByKey(pin);
        if(dev == NULL || dev->getDirection() == DIR_IN) return;
#ifdef DEVICE_ANALOGOUT
        if(dev->getDirection() == DIR_OUT) {
            return dev->getReferences().out->write(newValue);
        }
#endif
        return dev->getReferences().pwm->write(newValue);
    }

    AnalogPinReference* getAnalogGPIO(pinid_t pin) {
        return devices.getByKey(pin);
    }
};

#else

#ifdef ESP32
#include <SimpleCollections.h>
#include <driver/dac.h>
#define GPIO_INVALID -1

class EspAnalogOutputMode {
private:
    pinid_t pin;
    uint16_t pwmChannel;
    uint16_t pwmWidth;
public:
    EspAnalogOutputMode() {
        pin = GPIO_INVALID;
        pwmChannel = -1;
        pwmWidth = 5000;
    }
    EspAnalogOutputMode(const EspAnalogOutputMode& other) {
        pin = other.pin;
        pwmChannel = other.pwmChannel;
        pwmWidth = other.pwmWidth;
    }

    bool isDac() const {
        return pin == DAC1 || pin == DAC2;
    }

    pinid_t getKey() const {
        return pin;
    }

    uint16_t getPwmChannel() const {
        return pwmChannel;
    }

    void pinSetup(int pin_, int pwmChannel_) {
        this->pin = pin_;
        this->pwmChannel = pwmChannel_;
        if(!isDac()) {
            // for other than the dac ports, we need to set up PWM
            ledcSetup(pwmChannel, pwmWidth, 8);
            ledcAttachPin(pin, pwmChannel);

        }
        else {
            dac_output_enable(pin == DAC1 ? DAC_CHANNEL_1 : DAC_CHANNEL_2);
        }

    }

    void write(unsigned int newVal) const {
        if(isDac()) {
            dac_output_voltage(pin == DAC1 ? DAC_CHANNEL_1 : DAC_CHANNEL_2, newVal);
        }
        else {
            ledcWrite(pwmChannel, newVal);

        }
    }
};
#endif

/**
 * Creates an analog device that uses the core Arduino analog capabilities of ADC for reading
 * values and PWM or the inbuilt DAC if available for writing values. Generally speaking one
 * starts by initialising a pin as either input or output, then calling the read and write
 * methods to work with the device.
 *
 * If performance is not critical, use the floating point versions of the read and write methods
 * as these abstract away the absolute ranges, to 0 being GND and 1 being maximum voltage.
 *
 * Get an instance by calling internalAnalogIO() rather than creating one
 */
class ArduinoAnalogDevice : public AnalogDevice {
private:
	uint8_t readBitResolution;
	uint8_t writeBitResolution;
public:
    static ArduinoAnalogDevice* theInstance;

    /**
	 * Initialise the Arduino analog device with a given read and write bit resolution, on AVR and
	 * ESP8266 input is set to 10 bits (1024) and output to 8 bits (255). However, on ESP32 it
	 * is 12 bit input, 8 output. On SAMD and some other board types you can configure either 8, 10 or 12 bit.
	 * On SAMD by default 12 bit (4096) input 10 bit output.
	 */
	explicit ArduinoAnalogDevice(uint8_t readBitResolution = IOA_ANALOGIN_RES, uint8_t writeBitResolution = IOA_ANALOGOUT_RES) {
#if IOA_ANALOGIN_RES > 10
	    // some boards have the option for greater analog resolution on input. It needs to be configured
		analogReadResolution(readBitResolution);
#endif
#if (IOA_ANALOGOUT_RES > 8) && !defined(ESP8266)
		// some boards have the option for greater analog output resolution, but then it needs configuring.
		// except on esp8266 where 1024 pwm resolution is standard
		analogWriteResolution(writeBitResolution);
#endif
        this->readBitResolution = readBitResolution;
        this->writeBitResolution = writeBitResolution;
        theInstance = this;
	}

	int getMaximumRange(AnalogDirection dir, pinid_t /*pin*/) override {
		return (dir == DIR_OUT) ? (1 << writeBitResolution) : (1 << readBitResolution);
	}

    int getBitDepth(AnalogDirection direction, pinid_t /*pin*/) override {
        return (direction == DIR_IN) ? readBitResolution : writeBitResolution;
    }

	unsigned int getCurrentValue(pinid_t pin) override {
		return analogRead(pin);
	}

	float getCurrentFloat(pinid_t pin) override {
        auto maxValue = (float)getMaximumRange(DIR_IN, pin);
        return float(analogRead(pin)) / maxValue;
	}

	void setCurrentFloat(pinid_t pin, float value) override {
	    if(value < 0.0 || value > 1.0) value = 0;
	    float maxValue = getMaximumRange(DIR_OUT, pin);
	    auto compVal = (int)(value * maxValue);
	    setCurrentValue(pin, compVal);
	    //serdebugF4("Flt set ", value, maxValue, compVal);
	}
#ifdef ESP32
    void initPin(pinid_t pin, AnalogDirection direction) override {
        if(direction != DIR_IN) {
            EspAnalogOutputMode outputMode;
            outputMode.pinSetup(pin, gpioToPwmKey.count());
            gpioToPwmKey.add(outputMode);
        }
        else pinMode(pin, INPUT);
	}

	void setCurrentValue(pinid_t pin, unsigned int newVal) override {
	    auto output = gpioToPwmKey.getByKey(pin);
	    if(output) {
	        output->write(newVal);
	    }
    }

    EspAnalogOutputMode* getEspOutputMode(pinid_t pin) {
	    return gpioToPwmKey.getByKey(pin);
	}
private:
    BtreeList<pinid_t,EspAnalogOutputMode> gpioToPwmKey;
#else
    void initPin(pinid_t pin, AnalogDirection direction) override {
        pinMode(pin, (direction == DIR_IN) ? INPUT : OUTPUT);
    }

    void setCurrentValue(pinid_t pin, unsigned int newVal) override {
        analogWrite(pin, newVal);
    }
#endif
};

/**
 * Create an instance of the analog IO abstraction for the current hardware and cache it
 * so further calls return the same one, use this instead of creating one.
 * @return the analog device as a pointer.
 */
AnalogDevice* internalAnalogIo();

#endif // MBED or ARDUINO IMPL

#endif //_ANALOG_DEVICE_ABSTRACTION_H_
