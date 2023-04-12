/*
 * This file shows how to use IoAbstraction and switches to manage buttons on the ESP32 touch pins. These pins
 * when configured for touch, pick up changes in capacitance on the pin and turn it into a readable value and
 * IoAbstraction interprets these as either the switch being pressed or not.
 *
 * You need to read the main ESP32 guide to understand how the underlying functionality works, as this support is
 * directly based upon that, see link below:
 *  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/touch_pad.html
 *
 * More information about IoAbstraction:
 *  https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
 *
 * This uses output serial logging, see the logging guide:
 *  https://www.thecoderscorner.com/products/arduino-libraries/io-abstraction/arduino-logging-with-io-logging/
 */

// Enable this flag only to debug the touch values and determine the right settings.
// IE this will give you the threshold value and also the touch activation mode
//#define TOUCH_DEBUG_MODE

#include <IoAbstraction.h>
#include <TaskManagerIO.h>
#include <IoLogging.h>
#include <esp32/ESP32TouchKeysAbstraction.h>

#define LEFT_SENSOR 2
#define RIGHT_SENSOR 6
#define UP_SENSOR 5
#define DOWN_SENSOR 7
#define MY_TOUCH_THRESHOLD 800

//
// Here we initialise the touch keys for ESP32, the values are the high and low voltages ranges and the ADC attenuation
// settings, these are directly from the ESP32 includes, and I recommend you read the page linked in the top comment
// where all of these values are explained.
//
ESP32TouchKeysAbstraction touchKeys(800, TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

//
// This is an encoder emulation based around up and down buttons that also controls left and right too, when you change
// the intention to sideways, the left and right change the encoder value instead of up and down.
//
EncoderUpDownButtons *upDownEncoder = nullptr;

//
// Here we create a switch listener class, this is like an observer that is registered with switches further down,
// and will be notified of any changes in key state. Here we use the OO method of registering for presses instead of
// using the function callback approach.
//
class MySwitchListener : public SwitchListener {
public:
    void onPressed(pinid_t pin, bool held) override {
        serlogF3(SER_DEBUG, "Pressed ", pin, held);
    }

    void onReleased(pinid_t pin, bool held) override {
        serlogF3(SER_DEBUG, "Released ", pin, held);

        // when a pin is held and we're in encoder mode, be ready to change state!
        if(!held || upDownEncoder == nullptr) return;

        if(pin == LEFT_SENSOR) {
            // holding down left switches the encoder to scroll mode, this reverses the direction of the up and down
            // buttons to make scrolling feel more natural.
            upDownEncoder->setUserIntention(SCROLL_THROUGH_ITEMS);
            serlogF(SER_DEBUG, "scroll mode");
        } else if(pin == RIGHT_SENSOR) {
            // holding down right switches the encoder to scroll sideways, this means left and right reduce and increase
            // the encoder, UP becomes LEFT, right becomes DOWN.
            upDownEncoder->setUserIntention(SCROLL_THROUGH_SIDEWAYS);
            serlogF(SER_DEBUG, "side mode");
        }
    }
} mySwitchListener;

//
// This is called whenever the encoder changes, you could also use an EncoderListener OO callback (see other examples)
//
void encoderChangeFn(int newVal) {
    serlogF2(SER_DEBUG, "Encoder change ", newVal);
}

void setupForDirectSwitchAccess() {
    upDownEncoder = nullptr;

    // here we simply add all the sensors as switches, the callback above will receive the key state changes.
    switches.addSwitchListener(UP_SENSOR, &mySwitchListener, 20);
    switches.addSwitchListener(DOWN_SENSOR, &mySwitchListener, 20);
    switches.addSwitchListener(RIGHT_SENSOR, &mySwitchListener, 20);
    switches.addSwitchListener(LEFT_SENSOR, &mySwitchListener, 20);
}

void setupForEncoder() {
    // create a 4 way joystick instance, giving the 4 sensors to use, our OO key listener callback, and the function to
    // call when the encoder changes (You can also provide an OO EncoderListener if you prefer).
    upDownEncoder = new EncoderUpDownButtons(UP_SENSOR, DOWN_SENSOR, LEFT_SENSOR, RIGHT_SENSOR, &mySwitchListener, encoderChangeFn);
    // set the range to 100, starting point 50, allow wrapping around.
    upDownEncoder->changePrecision(100, 50, true);
    // and ensure interrupts are registered if needed.
    touchKeys.ensureInterruptRegistered();
}

void setup() {
    Serial.begin(115200);
    touchKeys.setTouchTriggerMode(TOUCH_TRIGGER_BELOW);

    // start up switches, provide an IoRef to the global touchkeys object, and choose the interrupt mode you desire.
    // the last parameter is the logic used, true=pullup. Only pull up logic is supported by this abstraction.
    switches.init(asIoRef(touchKeys), SWITCHES_POLL_EVERYTHING, true);

    // note only have one of the two calls below active.

    // Choose which function to use (only ONE line uncommented), either directly as switches, or as a 4 way encoder joystick.
    //setupForDirectSwitchAccess();
    setupForEncoder();
}

void loop() {
    taskManager.runLoop();
}