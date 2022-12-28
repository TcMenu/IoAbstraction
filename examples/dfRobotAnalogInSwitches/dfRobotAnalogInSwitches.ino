/**
 * This example shows how to use the switches library with a DfRobot shield. These shields
 * usually contain an LCD display combined with some buttons that are all connected onto a
 * single analog input. This abstraction turns the analog input back into a normalised
 * digital form that can be used with switches library. 
 * 
 * To keep the example as simple as possible, the inbuilt LCD is not used, preferring to
 * instead just output to the serial port.
 * 
 * See: https://www.dfrobot.com/wiki/index.php/Arduino_LCD_KeyPad_Shield_(SKU:_DFR0009)
 * for more information about these shields.
 *
 * Documentation and reference:
 *
 * https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
 * https://www.thecoderscorner.com/ref-docs/ioabstraction/html/index.html
 */

#include <IoAbstraction.h>
#include <DfRobotInputAbstraction.h>
#include <TaskManagerIO.h>

// The actual code that deals with the shield, this is the input abstraction that reads the button state from the
// analog input. The ranges are configurable and there are two defaults, both listed below.
DfRobotInputAbstraction dfRobotKeys(dfRobotAvrRanges); // or dfRobotV1AvrRanges

void logKeyPressed(const char* whichKey, bool heldDown) {
    Serial.print("Key ");
    Serial.print(whichKey);
    Serial.println(heldDown ? " Held" : " Pressed");
}

/**
 * Along with using functions to receive callbacks when a button is pressed, we can
 * also use a class that implements the SwitchListener interface. Here is an example
 * of implementing that interface. You have both choices, function callback or
 * interface implementation.
 */
class MyKeyListener : public SwitchListener {
private:
    const char* whatKey;
public:
    // This is the constructor where we configure our instance
    MyKeyListener(const char* what) {
        whatKey = what;
    }

    // when a key is pressed, this is called
    void onPressed(pinid_t /*pin*/, bool held) override {
        logKeyPressed(whatKey, held);
    }

    // when a key is released this is called.
    void onReleased(pinid_t /*pin*/, bool held) override {
        Serial.print("Release ");
        logKeyPressed(whatKey, held);
    }
};

MyKeyListener selectKeyListener("SELECT");

void setup() {
    // start up the serial port in a way compatible with 32 bit boards.
    while(!Serial);
    Serial.begin(115200);

    // initialise the switches component with the DfRobot shield as the input method.
    switches.initialise(asIoRef(dfRobotKeys), false); // df robot is always false for 2nd parameter.

    // now we add the directional switches, each one just logs the key press, the last parameter to addSwitch
    // is the repeat frequency is optional, set to NO_REPEAT and you'll get one additional call back for held state.
    const int repeatFrequencyTicks = 20;
    switches.addSwitch(DF_KEY_DOWN, [](pinid_t /*pin*/, bool held) { logKeyPressed("DOWN", held);}, repeatFrequencyTicks);
    switches.addSwitch(DF_KEY_UP, [](pinid_t /*pin*/, bool held) { logKeyPressed("UP", held);}, repeatFrequencyTicks);
    switches.addSwitch(DF_KEY_LEFT, [](pinid_t /*pin*/, bool held) { logKeyPressed("LEFT", held);}, repeatFrequencyTicks);
    switches.addSwitch(DF_KEY_RIGHT, [](pinid_t /*pin*/, bool held) { logKeyPressed("RIGHT", held);}, repeatFrequencyTicks);

    // here we add an onRelease callback, it will fire when the key is released
    switches.onRelease(DF_KEY_RIGHT, [](pinid_t /*pin*/, bool) { Serial.println("RIGHT has been released");});

    // lastly, we add an OO callback for the select key, see the MyKeyListener class defined above.
    switches.addSwitchListener(DF_KEY_SELECT, &selectKeyListener);
}

void loop() {
    taskManager.runLoop();
}