#line 2 "switchesTests.ino"

#include <AUnit.h>
#include "MockIoAbstraction.h"
#include "SwitchInput.h"

bool pressed;
uint8_t key;
bool held;
int callsMade;

using namespace aunit;

// this allows us to call the switches interrupt handler
extern void onSwitchesInterrupt(uint8_t);

void setup() {
    Serial.begin(115200);
    while(!Serial); // needed for some 32 bit boards.
}

void loop() {
	TestRunner::run();
}

void onSwitchPressed(uint8_t k, bool h) {
    callsMade++;
    key = k;
    held = h;
    pressed = true;
}

class SwitchesFixture : public TestOnce {
protected:
    MockedIoAbstraction mockIo;

public:
    SwitchesFixture() : mockIo(25) { }

    void setup() override {   
        pressed = false;
        held = false;
        callsMade = 0;
    }

    void assertPressedState(bool shouldBePressed) {
        // wait until the call back fires or we time out
        int loopCount = 0;
        while(pressed != shouldBePressed && ++loopCount < 200) {
            if(switches.isInterruptDriven()) mockIo.getInterruptFunction()();
            taskManager.yieldForMicros(20000);
        }    
        // now check if we are pressed, and if there are any errors.
        assertEqual(pressed, shouldBePressed);
        assertEqual(mockIo.getErrorMode(), NO_ERROR);
    }

    void assertHeldState(bool shouldBeHeld) {
        int loopCount = 0;
        while(held != shouldBeHeld && ++loopCount < 500) {
            taskManager.yieldForMicros(1000);
        }

        // now check if we are held and for any errors
        assertEqual(held, shouldBeHeld);
        assertEqual(mockIo.getErrorMode(), NO_ERROR);
    }
};

testF(SwitchesFixture, testPressingASingleButton) {
    switches.initialise(&mockIo, true);
    switches.addSwitch(2, onSwitchPressed, NO_REPEAT);

    // simulate the button being pressed (with a bounce).
    mockIo.setValueForReading(0, 0x0000); // pressed
    assertFalse(pressed); 
    mockIo.setValueForReading(1, 0x0004); // bounce 

    // we now simulate the button being pressed down.
    for(int i=2; i<25;i++)  mockIo.setValueForReading(i, 0x0000);
    assertPressedState(true);

    long millisStart = millis();

    // we now simualte the button being held, long press
    mockIo.resetIo();
    for(int i=0; i<25;i++)  mockIo.setValueForReading(i, 0x0000);
    assertHeldState(true);

    // make sure getting to held took near to 400 millis.
    assertMoreOrEqual(millis() - millisStart, (uint32_t)380);
    assertLess(millis() - millisStart, (uint32_t)450);
}

testF(SwitchesFixture, testInterruptButtonRepeating) {
    switches.initialiseInterrupt(&mockIo, true);
    switches.addSwitch(2, onSwitchPressed, 10);
    assertTrue(mockIo.isIntRegisteredAs(2, CHANGE));

    // simulate the button being pressed (with a double bounce - debounce 2).
    mockIo.setValueForReading(0, 0x0000);
    mockIo.setValueForReading(1, 0x0004); 
    mockIo.setValueForReading(2, 0x0000);
    mockIo.setValueForReading(3, 0x0004); 

    // we now simulate the button being pressed down (and the interrupt handler).
    for(int i=4; i<25;i++)  mockIo.setValueForReading(i, 0x0000);
    onSwitchesInterrupt(2);
    assertPressedState(true);

    // the first time around, the repeat threshold is 400milli, after that it is 200 millis
    // because we set the repeat interval to 10 of the switch, and the loop delay is 20millis.
    int threshold = 400;

    // try a few repeat keys..
    for(int i=0; i<4; i++) {
        // capture when we started
        long timeThen = millis();
        // we now simualte the button being held for a while
        mockIo.resetIo();
        for(int j=0; j<25;j++)  mockIo.setValueForReading(j, 0x0000);
        assertHeldState(true);

        // ensure it is repeating - IE the calls made should be near to the count.
        assertMoreOrEqual(callsMade, i);

        // ensure the timing is close to what we expect.
        assertMoreOrEqual(millis() - timeThen, (uint32_t)threshold  - 50);
        assertLess(millis() - timeThen, (uint32_t)threshold + 50);

        // after the first go, the timing should go down to 200 millis
        threshold = 200;
    }

    // keep track of the number of calls before button release
    int callsWhenButtonReleased = callsMade;

    // now simulate letting go of the button on the IO device.
    mockIo.resetIo();
    for(int j=0; j<25;j++)  mockIo.setValueForReading(j, 0x0004);
    mockIo.getInterruptFunction()();

    // wait long enough for switches to have triggered at least once.
    taskManager.yieldForMicros(32000);

    // we should not have received any more events.
    assertEqual(callsMade, callsWhenButtonReleased);
}
