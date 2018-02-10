# BasicIoAbstraction

This library provides several useful extensions that make programming Arduino for non-trivial tasks simpler. There are many different practical and familiar examples packaged with it in the `examples` folder. Below covers each of the main cases in more detail.

# TaskManager

Is a very simple scheduler that can be used to schedule things to happen either once or repeatedly in the future. Very similar to using setTimeout in Javascript or the executor framework in other languages. It also simplifies interrupt handling such that you are not in an ISR when called back, meaning you can do everything exactly as normal. The only real restriction with this library is not to call delay() or do any operations that block for more than a few microseconds. 

A simple example:

In the setup method, add an event that gets fired once in the future:

```
	taskManager.scheduleOnce(100, [] {
		// some work to be done.
	});
```
Then in the loop method you need to call: 

  	taskManager.runLoop();

# IoAbstraction

Lets you choose to use Arduino pins, shift register Input/Output, 8574 i2c IO Expanders in an inter-changable way. Use it in your sketch to treat shift registers or i2c expanders like pins. If you are building a library and want it to work with either Arduino pins, shift registers or an IO expander for IO, then this library is probably a good starting point.

A simple example:

At the global level (outside of any function) we create an i2c expander on address 0x20:

  	IoAbstractionRef ioExpander = ioFrom8754(0x20);

In setup we set it's first IO pin to input and start the Wire library:
	
  	Wire.begin();  
 	ioDevicePinMode(ioExpander, 0, INPUT);
  
And then later we red from it (the only limitation is we must call runLoop to synchronize the device state. This allows us to be efficient where possible, setting several pins, syncing and then reading pins.

  	ioDeviceSync(ioExpander);
  	int valueRead = ioDeviceDigitalRead(ioExpander, 0);

# SwitchInput

This class provides an event based approach to handling switches and rotary encoders. It full debounces switches before calling back your event handler and handles both repeat key and held down states. In the case of rotary encoders an interrupt on PIN_A is required, as the library needs to react very quickly; it is also important to make sure you have no long running tasks, or you'll miss the delayed rise. Note that this library also uses the above on task manager.

Here's a simple example example using a switch:

In setup we initialise it telling it to use arduino pins for IO, we could use shift registers or an i2c expander, and we also add a switch along with the event that should be:

	switches.initialise(ioUsingArduino());
	switches.addSwitch(spinwheelClickPin, onClicked, NO_REPEAT); // NO_REPEAT is optional, sets the repeat interval in 100s of second.

Then we create a function for onClicked, this will be called when the button is pressed:

	void onClicked(uint8_t pin, bool heldDown) {
		// pin: the pin that was pressed
    		// heldDown: if the button has been held down
  	}
  
Lastly, in loop you must not do anything long running, instead using the task management library. You must call:

	taskManager.runLoop();

# More detail

There's more detail here:

[http://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/]

[http://www.thecoderscorner.com/electronics/microcontrollers/switches-inputs/basic-io-abstraction-library-pins-or-8574/]

