/**
 * This is another example of using the DFRobot library, this time in conjunction with it's display. For the
 * simplest possible example, see simpleDfRobotShield example.
 * 
 * It expects that you include a liquid crystal library that you have available. I will assume you are using
 * the default one that's shipped with the IDE.
 * 
 * Shows the value of a rotary encoder on the display based on the UP and DOWN buttons. Select is also handled.
 */


// note you can switch this to include <LiquidCrystal.h> instead, just change the construction of lcd too.
#include <LiquidCrystal.h>
#include <IoAbstraction.h>
#include <DfRobotInputAbstraction.h>

// As per the above wiki this uses the default settings for analog ranges.
IoAbstractionRef dfRobotKeys = inputFromDfRobotShield();

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
bool repaintNeeded = true;

//
// This method is called by task manager on a regular basis, 250 millis to be exact.
// If there are changes to be painted it will re-paint the rotary encoder value.
// Never attempt to repaint something to an LCD too frequently, they are very slow.
//
void repaint() {
    if(!repaintNeeded) return;

    // here we get the current value from the encoder.
    int reading = switches.getEncoder()->getCurrentReading();

    // a quick and quite ugly way to zero pad a numeric value to four letters.
    lcd.setCursor(0, 1);
    int divisor = 1000;
    while(divisor > 0) {
        lcd.write((reading / divisor) + '0');
        reading = reading % divisor;
        divisor = divisor / 10;
    }

    repaintNeeded = false;
}

void setup() {
    //
    // set up the display and print our title on the first line
    //
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Rotary encoder:");

    // set up switches to use the DfRobot input facilities
    switches.initialise(dfRobotKeys, false);
    
    // we setup a rotary encoder on the up and down buttons
    setupUpDownButtonEncoder(DF_KEY_UP, DF_KEY_DOWN, [](int){repaintNeeded = true;});

    // with a maximum value of 5000, starting at 2500.
    switches.changeEncoderPrecision(5000, 2500);
    
    // now we add a switch handler for the select button
    switches.addSwitch(DF_KEY_SELECT, [](uint8_t, bool held) {
        lcd.setCursor(12, 1);
        lcd.print(held ? "HELD" : "PUSH");
    });

    // and we also want to know when it's released.
    switches.onRelease(DF_KEY_SELECT, [](uint8_t, bool) {
        lcd.setCursor(12, 1);
        lcd.print("    ");
    });

    // lastly, we set up a task that does the drawing for us every 250 millis.
    taskManager.scheduleFixedRate(250, repaint);
}

void loop() {
    taskManager.runLoop();
}