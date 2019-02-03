/**
 * This example shows how to use the switches library with a DfRobot shield. These sheilds
 * usually contain an LCD display combined with some buttons that are all connected onto a
 * single analog input. This abstraction turns the analog input back into a normalised
 * digital form that can be used with switches library. 
 * 
 * To keep the example as simple as possible, the inbuilt LCD is not used, preferring to
 * instead just output to the serial port.
 * 
 * See: https://www.dfrobot.com/wiki/index.php/Arduino_LCD_KeyPad_Shield_(SKU:_DFR0009)
 * for more information about these shields.
 */

#include <IoAbstraction.h>
#include <DfRobotInputAbstraction.h>

// As per the above wiki this uses the default settings for analog ranges.
IoAbstractionRef dfRobotKeys = inputFromDfRobotShield();

// for V1.0 of the shield uncomment the below definition and comment out the above defintion
// this has the other settings for analog ranges.
//IoAbstractionRef dfRobotKeys = inputFromDfRobotShieldV1();

void logKeyPressed(const char* whichKey, bool heldDown) {
    Serial.print("Key ");
    Serial.print(whichKey);
    Serial.println(heldDown ? " Held" : " Pressed");
}

void setup() {
    // start up the serial port in a way compatible with 32 bit boards.
    while(!Serial);
    Serial.begin(115200);

    // initialise the switches component with the DfRobot shield as the input method.
    switches.initialise(dfRobotKeys, false); // df robot is always false for 2nd parameter.

    // now we add the switches, each one just logs the key press, the last parameter to addSwitch
    // is the repeat frequency is optional, when not set it implies not repeating.
    switches.addSwitch(DF_KEY_DOWN, [](uint8_t key, bool held) { logKeyPressed("DOWN", held);}, 20);
    switches.addSwitch(DF_KEY_UP, [](uint8_t key, bool held) { logKeyPressed("UP", held);}, 20);
    switches.addSwitch(DF_KEY_LEFT, [](uint8_t key, bool held) { logKeyPressed("LEFT", held);}, 20);
    switches.addSwitch(DF_KEY_RIGHT, [](uint8_t key, bool held) { logKeyPressed("RIGHT", held);}, 20);
    switches.addSwitch(DF_KEY_SELECT, [](uint8_t key, bool held) { logKeyPressed("SELECT", held);});
    
    // and for the select button, lets add a release handler too.
    switches.onRelease(DF_KEY_SELECT, [](uint8_t, bool) { Serial.println("SELECT has been released");});
}

void loop() {
    taskManager.runLoop();
}