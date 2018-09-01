/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef _IO_ABSTRACTION_CORE_TYPES
#define _IO_ABSTRACTION_CORE_TYPES

/**
 * @file BasicIoAbstraction.h
 * 
 * Provides the core IoAbstraction interface and Arduino implementation of that interface.
 */


/** 
 * the definition of an interrupt handler function, to be called back when an interrupt occurs.
 */
typedef void (*RawIntHandler)(void);

/**
 * This class provides the interface by which all `IoAbstractionRef` types work. It makes it possible to
 * treat many types of IO in the same way, by providing a standard way of dealing with Arduino pins, 
 * shift registers and IO expanders.
 * 
 * As well as providing the standard interface, it also provides the Arduino pin implementation.
 * 
 * Normally, to use an IoAbstraction one would use the helper functions available, for this IoAbstraction
 * the helper function is `ioUsingArduino`
 */
class BasicIoAbstraction {
public:
	virtual ~BasicIoAbstraction() { }

	/**
	 * sets the pin direction for a pin controlled by this abstraction - as per `pinMode`
	 * @param pin the pin to be changed
	 * @param mode the new mode, as per pinMode
	 */
	virtual void pinDirection(uint8_t pin, uint8_t mode);

	/**
	 * Writes a value to a pin on this abstraction, as per `digitalWrite`. For serial devices, may need a sync first.
	 * @param pin the pin to be written to 
	 * @param value the new value such as HIGH / LOW
	 */
	virtual void writeValue(uint8_t pin, uint8_t value);

	/**
	 * Reads a value from a pin for this abstraction as per `digitalRead`. For serial devices may need a sync first.
	 * @param pin the pin to be read 
	 */
	virtual uint8_t readValue(uint8_t pin);
	
	/**
	 * Attach an interrupt to this abstraction, regardless of the device location this will perform the required tasks to register
	 * the interrupt in the requested mode. Note that not all devices can support all modes, for arduino pins all modes are supported. 
	 * @param pin the pin on this device to be used
	 * @param intHandler a void function with no parameters, used to handle interrupts. THIS IS A RAW INTERRUPT AND NOT MARSHALLED
	 * @param mode standard Arduino interrupt modes: CHANGE, RISING, FALLING
	 */
	virtual void attachInterrupt(uint8_t pin, RawIntHandler interruptHandler, uint8_t mode);

	/**
	 * This method is not needed on Arduino pins, but for most serial implementations it causes the device and abstraction to be synced.
	 */
	virtual void runLoop() { ; }

	/**
	 * Writes out a whole port at once, on Arduino pins this is achieved by providing any pin within that port.
	 * WARNING this is a potentially dangerous operation and should be used with care, as such there is no
	 * ioDevice helper function defined. Remember that you need to sync the device after using. 
	 * @param pin the pin determines the hardware port to use.
	 * @param portVal the 8 bit value to write to the port. Use with care. 
	 */
	virtual void writePort(uint8_t pin, uint8_t portVal);

	/**
	 * Reads a whole port at once, on Arduino pins this is achieved by providing any pin within that port.
	 * WARNING this is a potentially dangerous operation and should be used with care,  as such there is no
	 * ioDevice helper function defined. Remember that you need to sync the device before using.
	 * @param pin the pin determines the hardware port to use.
	 * @return the 8 bit value read from the port.
	 */
	virtual uint8_t readPort(uint8_t pin);
};

/** 
 * A reference to any type of IoAbstraction, that can be passed to one of the ioDevice.. functions. Normally, a reference
 * is obtained using a helper function, such as `ioUsingArduino`.
 */
typedef BasicIoAbstraction* IoAbstractionRef;

/**
 * Gives a reference to the Arduino pin implementation of IoAbstraction.
 */
IoAbstractionRef ioUsingArduino();

/**
 * Works in the same way as regular `pinMode` but this works for any IoAbstractionRef that you wish to set the pin mode on. 
 * @param ioDev the previously created IoAbstraction
 * @param the pin on the device to change mode 
 * @param the mode such as INPUT, OUTPUT, INPUT_PULLUP
 */
inline void ioDevicePinMode(IoAbstractionRef ioDev, uint8_t pin, uint8_t dir) { ioDev->pinDirection(pin, dir); }

/**
 * Works in the same way as `digitalRead`, but this works for any `IoAbstractionRef`, on the serial versions, the port is
 * cached, so before calling this method you need to have called `ioDeviceSync` first.
 * @see ioDeviceDigitalReadS for a shortcut version that syncs first, but is less efficient if there are many reads
 * @param ioDev the previously created IoAbstraction
 * @param the pin on the device to read 
 */
inline uint8_t ioDeviceDigitalRead(IoAbstractionRef ioDev, uint8_t pin) { return ioDev->readValue(pin); }

/**
 * Works in the same way as digitalWrite, but this works for any `IoAbstractionRef`, on the serial versions, the port is
 * cached, so before calling this method, you need to have called `IoDeviceSync` first.
 * @see ioDeviceDigitalWriteS for a shortcut version that syncs after writing, but is less efficient with many writes.
 * @param ioDev the previously created IoAbstraction
 * @param pin the pin to be updated
 * @param val the new value for the pin, HIGH/LOW
 */
inline void ioDeviceDigitalWrite(IoAbstractionRef ioDev, uint8_t pin, uint8_t val) { ioDev->writeValue(pin, (val)); }

/**
 * On serial versions of this abstraction, this causes synchronization to both the read and write values, so the chip and this IoAbstraction
 * are in sync.
 * @param ioDev the IoAbstraction to be synchronised.
 */ 
inline void ioDeviceSync(IoAbstractionRef ioDev) { ioDev->runLoop(); }

/**
 * Attach an interrupt to any IoAbstraction, regardless of the device location this will perform the required tasks to register
 * the interrupt in the requested mode. Note that not all devices can support all modes, check the device datasheet and the
 * particular abstraction you are using carefully, to ensure you get the expected result. 
 * @param ioDev the IoAbstraction to use
 * @param pin the pin on this device to be used
 * @param intHandler a void function with no parameters, used to handle interrupts. THIS IS A RAW INTERRUPT AND NOT MARSHALLED
 * @param mode standard Arduino interrupt modes: CHANGE, RISING, FALLING
 */
inline void ioDeviceAttachInterrupt(IoAbstractionRef ioDev, uint8_t pin, RawIntHandler intHandler, uint8_t mode) {ioDev->attachInterrupt(pin, intHandler, mode) ;}

/**
 * Works in the same way as `digitalRead`, but this works for any `IoAbstractionRef`, unlike the non 'S' version this automatically
 * calls ioDeviceSync first.
 * @see ioDeviceDigitalRead where you have more than one read / write to do at once, performs better in those cases.
 * @param ioDev the previously created IoAbstraction
 * @param the pin on the device to read 
 */
inline uint8_t ioDeviceDigitalReadS(IoAbstractionRef ioDev, uint8_t pin) { ioDev->runLoop(); return ioDev->readValue(pin); }

/**
 * Works in the same way as digitalWrite, but this works for any `IoAbstractionRef`, unlike the non 'S' version this automatically
 * calls ioDeviceSync first.
 * @see ioDeviceDigitalWrite where you have more than one read / write to do at once, performs better in those cases.
 * @param ioDev the previously created IoAbstraction
 * @param pin the pin to be updated
 * @param val the new value for the pin, HIGH/LOW
 */
inline void ioDeviceDigitalWriteS(IoAbstractionRef ioDev, uint8_t pin, uint8_t val) { ioDev->writeValue(pin, (val)); ioDev->runLoop(); }

#endif