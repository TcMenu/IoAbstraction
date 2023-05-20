//
// An example of using the MPR121 device with IoAbstraction, using this device for GPIO does not need any special
// calls, the library will initialise pins 4..11 for GPIO automatically as needed. However, to use the touch
// functions you will read the datasheet and understand the configuration properly. Most of the simpler setup has
// helper functions in the IoAbstraction, but we also expose the read/write register commands so that you can also
// perform custom setup.
//
// Documentation https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/
// DataSheet: https://www.sparkfun.com/datasheets/Components/MPR121.pdf
// Application note: https://www.nxp.com/docs/en/application-note/AN3894.pdf
//

#include <Arduino.h>
#include <Wire.h>
#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <SwitchInput.h>

// Create the abstraction itself and the analog wrapper, so we can dim LEDs using the analog abstraction easily
// and also read back raw sensor values from the analog abstraction.
MPR121IoAbstraction mpr121;
MPR121AnalogAbstraction mpr121Analog(&mpr121);

// When in setup mode, this just prints the raw sensor outputs for sensors 0..3
const bool setupMode = true;

void showRawSensorOutputs() {
    serdebugF("We are setting up in sensor setup mode");

    taskManager.scheduleFixedRate(500, [] {
        serlogF3("Filtered read 0 = ", mpr121Analog.getCurrentValue(0), mpr121Analog.getCurrentFloat(0));
        serlogF3("Filtered read 1 = ", mpr121Analog.getCurrentValue(1), mpr121Analog.getCurrentFloat(1));
    });
}

void setup() {
    // perform a reset of the device
    mpr121.softwareReset();
    delay(100);

    // set up touch pins 0 and 1, the first parameter is the pin
    mpr121.electrodeSettingsForPin(0, DEFAULT_ELECTRODE_CURRENT, DEFAULT_CHARGE_TIME, MY_TOUCH_THRESHOLD, MY_RELEASE_THRESHOLD);
    mpr121.electrodeSettingsForPin(1, DEFAULT_ELECTRODE_CURRENT, DEFAULT_CHARGE_TIME, MY_TOUCH_THRESHOLD, MY_RELEASE_THRESHOLD);

    // If needed you can adjust the debounce settings.
    //mpr121.configureDebounce(uint8_t debounceTouch, uint8_t debounceRelease);

    // if you need to you can access registers directly, there are ___8 and ___16 variants of read and write functions
    // This may be needed for more advanced configuration. For example..
    //mpr121.readReg8(MPR121_GPIO_DATA);

    if(setupMode) {
        showRawSensorOutputs();
    } else {
        // here we add two touchpad buttons on pins 0 and 1 and register them with switches, see the switches docs
        // for more on the switch configuration
        switches.init(asIoRef(mpr121), SWITCHES_POLL_EVERYTHING, true);
        switches.addSwitch(0, [](pinid_t key, bool held) {
            serdebugF3("Switch pressed ",  key, held);
        });
        switches.addSwitch(1, [](pinid_t key, bool held) {
            serdebugF3("Switch pressed ",  key, held);
        });

        // and now we setup pin 11 as LED output by using the analog abstraction we created earlier
        mpr121Analog.initPin(11, DIR_OUT);

        // cycle the LED current level between 0..250 using a task
        taskManager.scheduleFixedRate(100, [] {
            static int ledLevel = 0;
            ledLevel+=10;
            mpr121Analog.setCurrentValue(ledLevel);
        });

    }
}

void loop() {
    taskManager.runLoop();
}
