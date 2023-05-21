//
// An example of using the MPR121 device with IoAbstraction, using this device for GPIO does not need any special
// calls, the library will initialise pins 4..11 for GPIO automatically as needed. Supported configurations are either
// all 12 pins as capacitive touch or first 4 pins as capacitive touch and 8 GPIOs. Other variations are untested.
//
// However, to use the touch functions you will read the datasheet and understand the configuration properly. Most of
// the simpler setup has helper functions in the IoAbstraction, but we also expose the read/write register commands so
// that you can also perform custom setup.
//
// Using the touch functions with this library is a somewhat advanced topic, you will need to read the datasheet to
// understand how the device works and then configure the right settings for your case.
//
// Important, this sketch requires you enable IoLogging - see
// https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/arduino-logging-with-io-logging/
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

// You need to configure these by running the sketch in setup mode first and writing down the touch values for each.
#define MY_TOUCH_THRESHOLD 17
#define MY_RELEASE_THRESHOLD 60

// When in setup mode, this just prints the raw sensor outputs for sensors 0..3, you need to run this way first to
// get the thresholds for touching and releasing.
const bool setupMode = true;

// MPR121 pin setup
// this pin will be controlled using the LED controller
#define LED_PIN 11
// this pin will be used as GPIO input
#define INPUT_PIN 10
// these two will be used as touch input.
#define TOUCH_PIN_1 0
#define TOUCH_PIN_2 1
// the number of touch inputs, starting at ELE0, using more than 4(ELE3) turns off GPIO input support.
#define MAXIMUM_TOUCH_PIN 1

// Create the abstraction itself and the analog wrapper, so we can dim LEDs using the analog abstraction easily
// and also read back raw sensor values from the analog abstraction. You can also configure an interrupt pin if
// needed in the 2nd parameter, and even override wire, for example `mpr121(addr, intPin, &Wire2)`
MPR121IoAbstraction mpr121(0x5A);
MPR121AnalogAbstraction mpr121Analog(mpr121);

//
// Here we just start a task that runs frequently and prints out the filtered touch data, this should give you an
// idea what the threshold defines above should be set to.
//
void showRawSensorOutputs() {
    serdebugF("We are setting up in sensor setup mode");

    taskManager.scheduleFixedRate(1000, [] {
        serdebugF3("Filtered read 0 = ", mpr121Analog.getCurrentValue(0), mpr121Analog.getCurrentFloat(0));
        serdebugF3("Filtered read 1 = ", mpr121Analog.getCurrentValue(1), mpr121Analog.getCurrentFloat(1));

        // you can also read and write registers directly using read/writeReg in both 8 and 16 bit forms.
        // here's a few examples of reading the GPIO registers. Don't forget to write to anything other
        // than GPIO registers you must be in stop mode.
        serdebugF2("Data reg = ", mpr121.readReg8(MPR121_GPIO_DATA));
        serdebugF2("GPIOC0 reg = ", mpr121.readReg8(MPR121_GPIO_CONTROL_0));
        serdebugF2("GPIOC1 reg = ", mpr121.readReg8(MPR121_GPIO_CONTROL_1));
        serdebugF2("GPIOEn reg = ", mpr121.readReg8(MPR121_GPIO_ENABLE));
        serdebugF2("GPIODr reg = ", mpr121.readReg8(MPR121_GPIO_DIRECTION_0));
        serdebugF2("LEDPW0  reg = ", mpr121.readReg8(MPR121_LED_PWM_3));
        serdebugF2("Data reg = ", mpr121.readReg8(MPR121_GPIO_DATA));
    });
}

void keyReleased(pinid_t key, bool held) {
    serdebugF3("Release ", key, held);
}

void setup() {
    Serial.begin(115200);

    // This is configured for ESP32 but choose the right version for your board
    //Wire.begin(17, 5); // configuring with SDA and SCL for ESP32
    Wire.begin(); // using the default Wire configuration.

    serdebugF2("Starting MPR121 example setup mode = ", setupMode);

    // always follow this procedure to initialise the MPR121
    // 1. perform reset
    // 2. initialise touch controls
    // 3. call begin
    // 4. use abstraction as normal

    // perform a reset of the device and go into stop mode, small delay to let the device settle
    mpr121.softwareReset();
    delay(5);

    // set up touch pins 0 and 1, most of the time, you may only need to provide the first parameter (the pin) as the
    // others can be configured to use the defaults. Signature is:
    // electrodeSettingsForPin(pin, current=0, chargeTime=0, touchThreshold=0, releaseThreshold=0);
    mpr121.electrodeSettingsForPin(TOUCH_PIN_1, MY_TOUCH_THRESHOLD, MY_RELEASE_THRESHOLD);
    mpr121.electrodeSettingsForPin(TOUCH_PIN_2, MY_TOUCH_THRESHOLD, MY_RELEASE_THRESHOLD);

    // If needed you can adjust the debounce settings.
    //mpr121.configureDebounce(uint8_t debounceTouch, uint8_t debounceRelease);

    // now we start the device in the appropriate mode
    mpr121.begin(MAXIMUM_TOUCH_PIN, MPR121_MANUAL_CONFIG /*, configReg1=0x10, configReg2=0x20 */);

    if(setupMode) {
        showRawSensorOutputs();
    } else {
        // here we add two touchpad buttons on pins 0 and 1 and register them with switches, see the switches docs
        // for more on the switch configuration.
        bool pullUpMode = false; // do not use pull up for touch on MPR121
        switches.init(asIoRef(mpr121), SWITCHES_POLL_EVERYTHING, pullUpMode);
        switches.addSwitch(TOUCH_PIN_1, [](pinid_t key, bool held) {
            serdebugF3("Switch pressed ",  key, held);
        });
        switches.addSwitch(TOUCH_PIN_2, [](pinid_t key, bool held) {
            serdebugF3("Switch pressed ",  key, held);
        });
        switches.onRelease(TOUCH_PIN_1, keyReleased);
        switches.onRelease(TOUCH_PIN_2, keyReleased);
    }

    // set up pin 10 as regular input
    mpr121.pinMode(INPUT_PIN, INPUT);

    // and now we set up pin 11 as LED output by using the analog abstraction we created earlier
    mpr121Analog.initPin(LED_PIN, DIR_OUT);
    mpr121.digitalWriteS(LED_PIN, HIGH);

    // cycle the LED current level between 0..250 using a task
    taskManager.scheduleFixedRate(100, [] {
        static uint8_t ledLevel = 0;
        ledLevel+=10;
        mpr121Analog.setCurrentValue(LED_PIN, ledLevel);
    });

    // check the input now and again
    taskManager.scheduleFixedRate(1000, [] {
        auto gpioInState = mpr121.digitalRead(INPUT_PIN);
        serdebugF2("GPIO in = ", gpioInState);
    });

}

void loop() {
    taskManager.runLoop();
}
