# IoAbstraction for Arduino and mbed summary
[![PlatformIO](https://github.com/davetcc/IoAbstraction/actions/workflows/platformio.yml/badge.svg)](https://github.com/davetcc/IoAbstraction/actions/workflows/platformio.yml)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache--2.0-green.svg)](https://github.com/davetcc/IoAbstraction/blob/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/davetcc/IoAbstraction.svg?maxAge=3600)](https://github.com/davetcc/IoAbstraction/releases)
[![davetcc](https://img.shields.io/badge/davetcc-dev-blue.svg)](https://github.com/davetcc)
[![JSC electronics](https://img.shields.io/badge/JSC-electronics-green.svg)](https://github.com/jsc-electronics)

Dave Cherry / TheCodersCorner.com made this library available for you to use. It takes me significant effort to keep all my libraries current and working on a wide range of boards. Please consider making at least a one off donation via the sponsor button if you find it useful. In forks, please keep text to here intact.

This library provides several useful extensions that make programming Arduino / mbed for non-trivial apps simpler. There are many different practical and familiar examples packaged with it in the `examples` folder. Below I cover each of the main functions briefly with a link to more detailed documentation. The API is almost identical between Arduino and mbed making it easier to port between the two. 

## Documentation and questions

Along with ths quick start guide and the examples also see:

* [IoAbstraction documentation pages](https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/)
* [IoAbstraction reference documentation](https://www.thecoderscorner.com/ref-docs/ioabstraction/html)

Community questions can be asked in the discussions section of this repo, or using the Arduino forum. We generally answer most community questions but the responses will not be timely. Before posting into the community make sure you've recreated the problem in a simple sketch, and please consider making at least a one time donation (see links further up):

<a href="https://www.buymeacoffee.com/davetcc" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-blue.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

* [discussions section of the IoAbstraction repo](https://github.com/davetcc/IoAbstraction/discussions)
* [Arduino discussion forum](https://forum.arduino.cc/) where questions can be asked, please tag me using `@davetcc`.
* [Legacy discussion forum probably to be made read only soon](https://www.thecoderscorner.com/jforum/).

## Installation for Arduino IDE

This library is available in library manager on both Arduino and PlatformIO, this is the best choice for most people. It should automatically install the dependencies, [TaskManagerIO](https://github.com/davetcc/TaskManagerIO) and [SimpleCollections](https://github.com/davetcc/SimpleCollections). If for some reason it does not install the dependency, please also install it manually. It is highly recommended that you install the libraries using your library manager.

## Installation for PlatformIO (Arduino or mbed)

Use the platformIO library manager to get the library. It's called 'IoAbstraction'. It should automatically include "TaskManagerIO" and "SimpleCollections" as it's a dependency.

## This library is based on TaskManagerIO and SimpleCollections

Take a look at the [TaskManagerIO repo](https://github.com/davetcc/TaskManagerIO) for more information about how task manager works, this library relies heavily on task manager.

Also, this library uses [SimpleCollections](https://github.com/davetcc/SimpleCollections) within switches and a few other areas.

## BasicIoAbstraction - Arduino like interface to pins, PCF8574, PCF8575, MCP23017, AW9523 and shift registers.

Lets you choose to use Arduino pins, shift register Input/Output, I2C PCF8574/PCF8575, AW9523, and MCP23017 in an inter-changable way. Use it in your sketch to treat shift registers or i2c expanders like pins. There's even an abstraction that can combine together Arduino pins and one or more other expander! See the documentation (link further up) for more details.

If you are building a library and want it to work with either Arduino pins, shift registers or an IO expander for IO, then this library is probably a good starting point.

A simple example:

If we want to use the i2c wire based ioFrom8574 we must include the wire header file

	#include <IoAbstractionWire.h>

At the global level (outside of any function) we create an i2c expander on address 0x20:

	PCF8574IoAbstraction io8574(0x20, 0);
	PCF8574IoAbstraction io8575(address, interruptPin, wireInstance, mode16Bit, invertedLogic);
	
If you use an I2C variant, you must initialise wire before using

	Wire.begin();  

Or for Arduino pins instead simply call the following:
	
	internalDigitalDevice()
	
You can convert any Io-device object into an IoAbstractionRef as follows

	IoAbstractionRef myRef = asIoRef(io8574);

And lastly for DfRobot LCD shield input we use (requires library V1.3.2 at least):

	DfRobotInputAbstraction dfRobotKeys(dfRobotAvrRanges); // or dfRobotV1AvrRanges as appropriate

To configure pin direction on any expander, you use the following, although some devices are read or write only, you should still call pinMode:

 	ioExpander.pinMode(0, INPUT);
  
And then later we read from it, in this case as we are doing a single read, use the 'S' version of the method as it removes the need to call the sync method. The only limitation is we must synchronize the device state. This allows us to be efficient where possible, setting several pins, syncing and then reading pins.

  	int valueRead = ioDeviceDigitalReadS(ioExpander, 0); // read pin 0 on ioExpander

Let's now say we wanted to write one value and read two items on the same device, in this case we don't use the 'S' version of the method, because otherwise it would sync three times.

	ioExpander.digitalWrite(outputPin, HIGH);
	ioExpander.sync();
	int read1 = ioExpander.digitalRead(inputPin1);
	int read2 = ioExpander.digitalRead(inputPin2);

## SwitchInput - buttons that are debounced with event based callbacks

This class provides an event based approach to handling switches and rotary encoders. It fully de-bounces encoders and switches before calling back your event handler, while handling press, release, repeat key and held down states. It is however important to make sure you have no long-running tasks, or you'll miss the delayed rise. Note that this component also uses task manager.

Before doing anything else, you must add taskManager's run loop to your loop method, and your loop method must not do any long delay calls.

	void loop() {
		taskManager.runLoop();
	}

Here's a simple example using a switch:

In setup we initialise it telling it to use arduino pins for IO, we could use shift registers or an i2c expander, and we also add a switch along with the event that should be:

    // our next task is to initialise swtiches, do this BEFORE doing anything else with switches.
    // We choose to initialise in poll everything (requires no interrupts), but there are other modes too:
    // (SWITCHES_NO_POLLING - interrupt only) or (SWITCHES_POLL_KEYS_ONLY - encoders on interrupt) 
    switches.init(boardIo, SWITCHES_POLL_EVERYTHING, true);

    // NO_REPEAT is optional, sets the repeat interval in 100s of second.
	switches.addSwitch(spinwheelClickPin, onClicked, NO_REPEAT); 

Then we create a function for onClicked, this will be called when the button is pressed:

	void onClicked(uint8_t pin, bool heldDown) {
		// pin: the pin that was pressed
        // heldDown: if the button has been held down
  	}

It is also possible to use initialiseInterrupt instead of initialise, when using this mode the library does not poll the switches unless a button is pressed down. Its use is interchangeable with initialise().

## RotaryEncoder - hardware and button emulation, even available with i2c IO expanders

Switch input also fully supports rotary encoders (and simulated rotary encoders using up / down buttons). For this you just initialise the rotary encoder, and if you choose "poll everything" mode then PIN_A doesn't need an interrupt. However, in any other mode you must register an interrupt for PIN_A (even if the interrupt is shared). No debouncing is needed, the library will switch on pull up resistors too, but you may need lower resistance pull-ups will long wire runs.

For more see https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/arduino-switches-handled-as-events/.

First we must register the callback function that will be called when there's a change

	void onEncoderChange(int newValue) {
		// do something with new value..
	}

Then we create an encoder using one of the three examples below

	// Example 1, Real encoder, we need to set up the pins that the encoder uses and provide a callback
	setupRotaryEncoderWithInterrupt(encoderAPin, encoderBPin, onEncoderChange);
	
	// Example 2, Up / down buttons acting like an encoder
	setupUpDownButtonEncoder(pinUpBtn, pinDownBtn, onEncoderChange);

	// Example 3, advanced usage, same as example 1, but with two encoders
	HardwareRotaryEncoder* firstEncoder = new HardwareRotaryEncoder(firstEncoderAPin, firstEncoderBPin, onFirstEncoderChange);
	HardwareRotaryEncoder* secondEncoder = new HardwareRotaryEncoder(secondEncoderAPin, secondEncoderBPin, onSecondEncoderChange);
	switches.setEncoder(0, firstEncoder);
	switches.setEncoder(1, secondEncoder);

For the vast majority of encoders there is no need to provide the encoder type. If you have a quarter cycle rotary encoder, there is an extra optional constructor parameter for the encoder type, thanks go to @ddd999 for this support. The options are listed below: 

    /** Detent after every signal change, A or B */
    QUARTER_CYCLE,
    /** Detent on every position where A == B */
    HALF_CYCLE,
    /** Detent after every full cycle of both signals, A and B */ 
    FULL_CYCLE

Then lastly we set the precision of the encoder (IE the range), if the current and maximum value are both 1, then the mode is direction only.

	// After initialising, we set the maximum value (from 0) that the encoder represents
	// along with the current value
	switches.changeEncoderPrecision(maximumEncoderValue, 100);

	// advanced usage: if you want to change the precision of other than the first encoder
	switches.changeEncoderPrecision(1, maximumValue, currentValue);

### Notes for using more than rotary encoder at the same time

There are a few limitations with multiple encoders. Firstly, the encoders must all be on the same input device, such that the interrupt
comes from a device that is shared by them all. For example the they should all share an IO device such as a 23017 or an 8574, or if on 
arduino pins, all the A pins must be interrupt driven. Secondly, there is a hard limit on the number defined by `MAX_ROTARY_ENCODERS` which 
you can change by altering the file `SwitchInput.h` should you need more (or less) than 4.

## EepromAbstraction - support for both AVR and i2c AT24 EEPROMs with a common interface

The eeprom abstraction has several implementations, which makes it possible for libraries and code to be transparent from
AVR or AT24 based I2C eeprom storage, it even has a No-Op implementation as well. All the implementations shown below are interchangable
so if like me you switch between 8 and 32 bit boards, just change the EEPROM implementation!

### Avr Eeprom abstraction

This implementation uses the standard AVR EEPROM space for storage - only available on 8bit AVR such as Uno, MEGA.

To create an instance

	AvrEeprom avrEeprom;

### I2c AT24 EEPROM abstraction

A ground up implementation of the i2c eeprom protocol that should be compatible with the vast majority of i2c EEPROM devices.

To create an instance we pass the address of the chip (usually between 0x50 and 0x57), and also the page size, below are the page sizes 
for the most common devices. Consult the datasheet if unsure.

| ROM               | PageSize | Memory Bytes |
|-------------------|----------|--------------|
| PAGESIZE_AT24C01  | 8        | 128B         | 
| PAGESIZE_AT24C02  | 8        | 256B         | 
| PAGESIZE_AT24C04  | 16       | 512B         | 
| PAGESIZE_AT24C08  | 16       | 1kB          | 
| PAGESIZE_AT24C16  | 16       | 2kB          | 
| PAGESIZE_AT24C32  | 32       | 4kB          | 
| PAGESIZE_AT24C64  | 32       | 8KB          |
| PAGESIZE_AT24C128 | 64       | 16KB         |        
| PAGESIZE_AT24C256 | 64       | 32KB         |       
| PAGESIZE_AT24C512 | 128      | 64KB         |  


	I2cAt24Eeprom anEeprom(addressOfRom, pageSize);
 
Then during setup, you must ensure you call Wire.begin()

	void setup() {
		Wire.begin();
		
		// your other setup code.
	}
 
 
 ### NoEeprom - does nothing, but fulfills the interface.

Does nothing but implements the interface - useful sometimes..

To create an instance

	NoEeprom anEeprom;

 
### Reading and writing EEPROM values

Writing primitive values
 
	anEeprom.write8(romAddr, byteVal);
	anEeprom.write16(romAddr, value16);
	anEeprom.write32(romAddr, value32);

	byte by = anEeprom.read8(romStart);
	unsigned int i16 = anEeprom.read16(romStart);
	unsigned long i32 = anEeprom.read32(romStart);
	
Writing arrays and strings

	char data[20]; // example array to work with
	anEeprom.readIntoMemArray((unsigned char*)data, romStart, sizeof data);
	anEeprom.writeArrayToRom(romStart, (const unsigned char*)data, sizeof data);

### Analog device abstraction

Since 1.4 a new abstraction for analog devices has been added, it allows for an interchangable interface between most analog read and write devices such as ADC, DAC, PWM, Volume controls and Digital Potentiometers. At the moment the only available one is the Arduino pin based implementation. See the `analogExample` for usage.

Note that although the Arduino constructor allows the bit depth to be set, it only has any effect on SAMD boards. 

```
    // create the analog device
    ArduinoAnalogDevice analog;

    // to make A1 an input
    analog.initPin(A1, DIR_IN);
    
    // and make the PWM_PIN output.
    analog.initPin(PWM_PIN, DIR_OUT);

    // returns the range of the pin requested in the direction specified.
	int range = getMaximumRange(DIR_IN, A1);

    // to read from A1
    int reading = analog.getCurrentValue(A1);

    // to write to PWM_PIN
    analog.setCurrentValue(PWM_PIN, newValue);
```

## Matrix keyboard support

You can create matrix keyboards with any arrangement of keys, but the two most common cases of 3x4 and 4x4 layout number pads have ready-made layouts. There is an example showing usage in detail in both polling and interrupt mode on device pins and an I2C IoExpander.

Alongside the example, there is comprehensive documentation describing the use of [matrix keyboards on Arduino](https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/matrix-keyboard-keypad-manager/).  

## Inbuilt Unit testing framework and mocking

IoAbstraction has a very simple unit test framework built into it, it works on a very wide range of boards, more or less everything in our supported list. In addition to this there are mock versions of some components to make testing your code easier. See https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/simple-test-unit-test-arduino-mbed/

## ESP32 extras mode

On ESP32 we are slowly adding support for direct IDF, as it's been requested by one of our clients. This will slowly appear over several releases. To enable this mode you can set the flag `IOA_USE_ESP32_EXTRAS`. Once you do this IDF functions are used for all digital IO functions. We always use IDF functions for analog input and DAC output, and are slowly moving toward direct LTDC functions for PWM instead of wrappers. 

## Making changes to IoAbstraction

We welcome people rolling up their sleeves and helping out, but please do reach out to us before starting any work, so we can ensure it's in sync with our development. We use platformIO for development and have a specific project available to help you get started, along with tests that check many elements still work as expected. See [https://github.com/davetcc/tcLibraryDev]
