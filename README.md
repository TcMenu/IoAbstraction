# BasicIoAbstraction
This library abstracts the use of either arduino pins or IO expanders, such that a library can offer both choices. 

If you are building a library and want it to work with either Arduino pins or an IO expander for IO, then this library is probably a good starting point.

See the example that is packaged with it, it uses both an IO expander (8574) and Arduino pins as an example of the library.

## Simple i2c example:

    #include <BasicIoAbstraction.h>
    #include <Wire.h>


    BasicIoAbstraction* pins = ioFrom8754(0x20);


Then in setup:

	pins->pinDirection(0, INPUT);

Then in loop:

	pins->runLoop(); // send commands to the chip.
	uint8_t switchValue = pins->readValue(0);


There's more detail here:
[http://www.thecoderscorner.com/electronics/microcontrollers/switches-inputs/basic-io-abstraction-library-pins-or-8574/]
