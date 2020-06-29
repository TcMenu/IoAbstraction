#ifndef _ANALOG_DEVICE_ABSTRACTION_H_
#define _ANALOG_DEVICE_ABSTRACTION_H_

#ifdef __MBED__
#include <mbed.h>
#else
#include <Arduino.h>
#endif

#include <BasicIoAbstraction.h>

/** 
 * an enumeration that describes direction, eg input or output for the ADC/POT/DAC. For some
 * devices only one mode will make sense.
 */
enum AnalogDirection { DIR_IN, DIR_OUT, DIR_PWM };

/**
 * Describes an analog device that has commands to both read values from and write values to
 * a device. Not all devices will support both input and output. When such a case occurs the
 * getMaximumRange would return -1 for that direction. This abstraction can support ADC, PWM
 * DAC and Potentiometer devices.
 */
class AnalogDevice {
public:
	/**
	 * @param dir the direction required
	 * @param pin the pin for which the range is desired
	 * @return the maximum range of the analog input or output on this device for the given pin 
	 */
	virtual int getMaximumRange(AnalogDirection direction, uint8_t pin)=0;

    /**
     * @param pin the pin for which the bit depth is required.
     * @param direction the direction in which the depth is queried (DIR_IN, DIR_OUT)
     * @return the number of bits
     */
    virtual int getBitDepth(AnalogDirection direction, uint8_t pin) = 0;

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
	virtual void setCurrentValue(uint8_t pin, unsigned int newValue)=0;

	/**
	 * sets the current value based on a float from 0 to 1, where 0 is
	 * minimum and 1 is maximum.
	 * @param pin the pin for which to set
	 * @param newValue the new value which should be between 0 and 1.0
	 */
    virtual void setCurrentFloat(uint8_t pin, float newValue)=0;

};

#ifdef __MBED__

class AnalogPinReference {
private:
    pinid_t pin;
    AnalogDirection direction;
    union AnalogPinReferences {
        AnalogIn* input;
        AnalogOut* out;
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
            case DIR_OUT:
                analogRef.out = new AnalogOut((PinName)pin);
                break;
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
 */
class MBedAnalogDevice : public AnalogDevice {
private:
    BtreeList<pinid_t, AnalogPinReference> devices;
public:
    int getMaximumRange(AnalogDirection direction, uint8_t pin) override {
        return 0xffff;
    }

    int getBitDepth(AnalogDirection direction, uint8_t pin) override {
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

    void setCurrentValue(uint8_t pin, unsigned int newValue) override {
        auto dev = devices.getByKey(pin);
        if(dev == NULL || dev->getDirection() == DIR_IN) return;
        if(dev->getDirection() == DIR_OUT) {
            return dev->getReferences().out->write_u16(newValue);
        }
        else {
            return dev->getReferences().pwm->write(float(newValue) / 65535.0F);
        }
    }

    void setCurrentFloat(uint8_t pin, float newValue) override {
        auto dev = devices.getByKey(pin);
        if(dev == NULL || dev->getDirection() == DIR_IN) return;
        if(dev->getDirection() == DIR_OUT) {
            return dev->getReferences().out->write(newValue);
        }
        else {
            return dev->getReferences().pwm->write(newValue);
        }

    }
};

#else

/**
 * Creates an analog device that uses the core Arduino analog capabilities of ADC for reading
 * values and PWM or the inbuilt DAC on SAMD for writing values. It is a nice abstraction as
 * it would later allow to change to an offboard device without changing much code.
 */
class ArduinoAnalogDevice : public AnalogDevice {
private:
	uint8_t readBitResolution;
	uint8_t writeBitResolution;
public:
	/**
	 * Initialise the Arduino analog device with a given read and write bit resolution, usually
	 * input is set to 10 bits (1024) and output to 8 bits (255) by default. On SAMD and some
	 * other board types this can be significantly increased. 
	 * For example SAMD: 12 bit (4096) input and output.
	 */
	ArduinoAnalogDevice(uint8_t readBitResolution = 10, uint8_t writeBitResolution = 8) {
#ifdef ARDUINO_ARCH_SAMD
		analogReadResolution(readBitResolution);
		analogWriteResolution(writeBitResolution);
		this->readBitResolution = readBitResolution;
		this->writeBitResolution = writeBitResolution;
#else
		this->readBitResolution = 10;
		this->writeBitResolution = 8;
#endif
	}

	int getMaximumRange(AnalogDirection dir, pinid_t /*pin*/) override {
		return (dir == DIR_OUT) ? (1 << writeBitResolution) : (1 << readBitResolution);
	}

    int getBitDepth(AnalogDirection direction, pinid_t /*pin*/) override {
        return (direction == DIR_IN) ? readBitResolution : writeBitResolution;
    }

	void initPin(pinid_t pin, AnalogDirection direction) override {
		pinMode(pin, (direction == DIR_IN) ? INPUT : OUTPUT);
	}

	unsigned int getCurrentValue(pinid_t pin) override {
		return analogRead(pin);
	}

	float getCurrentFloat(pinid_t pin) override {
        float maxValue = getMaximumRange(DIR_IN, pin);
        return analogRead(pin) / maxValue;
	}

	void setCurrentValue(pinid_t pin, unsigned int newVal) override {
		analogWrite(pin, newVal);
	}

	void setCurrentFloat(pinid_t pin, float value) override {
	    if(value < 0.0 || value > 1.0) value = 0;
	    float maxValue = getMaximumRange(DIR_OUT, pin);
	    auto compVal = (int)(value * maxValue);
	    analogWrite(pin, compVal);
	    //serdebugF4("Flt set ", value, maxValue, compVal);
	}
};

#endif // MBED or ARDUINO IMPL

#endif //_ANALOG_DEVICE_ABSTRACTION_H_
