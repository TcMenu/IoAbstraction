# BasicIoAbstraction

This library provides several useful extensions that make programming Arduino for non-trivial tasks simpler. There are many different practical and familiar examples packaged with it in the `examples` folder. Below covers each of the main cases in more detail.

# TaskManager

Is a very simple scheduler that can be used to schedule things to happen either once or repeatedly in the future. Very similar to using setTimeout in Javascript or the executor framework in other languages. It also simplifies interrupt handling such that you are not in an ISR when called back, meaning you can do everything exactly as normal. The only real restriction with this library is not to call delay() or do any operations that block for more than a few microseconds. 

At the moment the two schedule calls are accurate to 1ms up to 1 second, and accurate to 8ms up to about 55 seconds.

# BasicIoAbstraction

Abstracts the use of either arduino pins or IO expanders, such that a library or other code can work very easily with shift registers, i2c IO or arduino pins. 

If you are building a library and want it to work with either Arduino pins or an IO expander for IO, then this library is probably a good starting point.

# SwitchInput

The first version of switch input is ready. It supports pull down switches only at the moment. This class provides an event based approach to handling switches and rotary encoders. In the case of rotary encoders an interrupt on PIN_A is required, as the library needs to react very quickly; it is also important to make sure you have no long running tasks, or you'll miss the delayed rise.

# More detail

There's more detail here:

[http://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/]

[http://www.thecoderscorner.com/electronics/microcontrollers/switches-inputs/basic-io-abstraction-library-pins-or-8574/]

