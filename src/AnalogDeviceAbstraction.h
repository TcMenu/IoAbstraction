#ifndef _ANALOG_DEVICE_ABSTRACTION_H_
#define _ANALOG_DEVICE_ABSTRACTION_H_

#include <Arduino.h>

/** 
 * an enumeration that describes direction, eg input or output for the ADC/POT/DAC. For some
 * devices only one mode will make sense.
 */
enum AnalogDirection { DIR_IN, DIR_OUT };

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
	 * initialises a pin as either an input or output of analog signals. No validation to check if
	 * that pin can support input or output is performed.
	 *
	 * @param pin the pin to initialise
	 * @param direction the direction required
	 */
	virtual void initPin(uint8_t pin, AnalogDirection direction) = 0;

	/**
	 * Returns the current value on the ADC for the given pin
	 * @param pin the pin to read from
	 * @return the current value on that pin
	 */
	virtual unsigned int getCurrentValue(uint8_t pin)=0;

	/**
	 * Sets the current value on an output capable device to a new value
	 * @param pin the pin to read from
	 * @param newValue the value to be set
	 */
	virtual void setCurrentValue(uint8_t pin, unsigned int newValue)=0;
};

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

	int getMaximumRange(AnalogDirection dir, uint8_t pin) override {
		return (dir == DIR_OUT) ? (1 << writeBitResolution) : (1 << readBitResolution);
	}

	void initPin(uint8_t pin, AnalogDirection direction) override {
		pinMode(pin, (direction == DIR_IN) ? INPUT : OUTPUT);
	}

	unsigned int getCurrentValue(uint8_t pin) override {
		return analogRead(pin);
	}

	void setCurrentValue(uint8_t pin, unsigned int newVal) override {
		analogWrite(pin, newVal);
	}
};

#endif //_ANALOG_DEVICE_ABSTRACTION_H_
