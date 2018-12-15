#line 2 "switchesTests.ino"

#include <AUnit.h>
#include "MockIoAbstraction.h"
#include "SwitchInput.h"

bool pressed;
bool keyReleased;
uint8_t key;
bool held;
int callsMade;
int encoderCurrentVal;

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

void encoderCallback(int newValue) {
    encoderCurrentVal = newValue;
    callsMade++;
}

void onSwitchPressed(uint8_t k, bool h) {
    callsMade++;
    key = k;
    held = h;
    pressed = true;
}

void onSwitchReleased(uint8_t k, bool h) {
    keyReleased = true;
    held = h;
}

class SwitchesFixture : public TestOnce {
protected:
    MockedIoAbstraction mockIo;

public:
    SwitchesFixture() : mockIo(25) { }

    void setup() override {   
        mockIo.resetIo();
        taskManager.reset();

        pressed = false;
        keyReleased = false;
        held = false;
        callsMade = 0;
        encoderCurrentVal = 0;
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
        assertFalse(keyReleased);
        assertEqual(mockIo.getErrorMode(), NO_ERROR);
    }

    void assertHeldState(bool shouldBeHeld) {
        int loopCount = 0;
        while(held != shouldBeHeld && ++loopCount < 500) {
            taskManager.yieldForMicros(1000);
        }

        // now check if we are held and for any errors
        assertEqual(held, shouldBeHeld);
        assertFalse(keyReleased);
        assertEqual(mockIo.getErrorMode(), NO_ERROR);
    }

    void assertReleasedState(bool expectedHeldState) {
        int loopCount = 0;
        while(!keyReleased && ++loopCount < 500) {
            taskManager.yieldForMicros(1000);
        }
        assertTrue(keyReleased);
        assertEqual(held, expectedHeldState);
    }

    void runInterruptLoopTimes(int times) {
        for(int i=0; i<times; i++) {
            mockIo.getInterruptFunction()();
            taskManager.yieldForMicros(10);
        }
    }
};

testF(SwitchesFixture, testPressingASingleButton) {
    switches.initialise(&mockIo, true);
    switches.addSwitch(2, onSwitchPressed, NO_REPEAT);
    switches.onRelease(2, onSwitchReleased);
    assertFalse(switches.isInterruptDriven());

    // simulate the button being pressed (with a bounce).
    mockIo.setValueForReading(0, 0x0000); // pressed
    mockIo.setValueForReading(1, 0x0000); // pressed
    assertFalse(pressed); 
    mockIo.setValueForReading(2, 0x0004); // bounce 

    // we now simulate the button being pressed down.
    for(int i=3; i<25;i++)  mockIo.setValueForReading(i, 0x0000);
    assertPressedState(true);
    assertFalse(keyReleased);

    long millisStart = millis();

    // we now simualte the button being held, long press
    mockIo.resetIo();
    for(int i=0; i<25;i++)  mockIo.setValueForReading(i, 0x0000);
    assertHeldState(true);

    // we now simulate the button being released.
    mockIo.resetIo();
    for(int i=0; i<25;i++)  mockIo.setValueForReading(i, 0x0004);
    assertReleasedState(true);


    // make sure getting to held took near to 400 millis.
    assertMoreOrEqual(millis() - millisStart, (uint32_t)380);
    assertLess(millis() - millisStart, (uint32_t)450);
}

testF(SwitchesFixture, testInterruptButtonRepeating) {
    // initialise the switches library using interrupt based initialisation.
    switches.initialiseInterrupt(&mockIo, true);
    switches.addSwitch(2, onSwitchPressed, 10);
    assertTrue(mockIo.isIntRegisteredAs(2, CHANGE));
    assertTrue(switches.isInterruptDriven());

    // simulate the button being pressed (with a double bounce - debounce 2).
    mockIo.setValueForReading(0, 0x0000);
    mockIo.setValueForReading(1, 0x0000);
    mockIo.setValueForReading(2, 0x0004); 
    mockIo.setValueForReading(3, 0x0000);
    mockIo.setValueForReading(4, 0x0004); 

    // we now simulate the button being pressed down (and the interrupt handler).
    for(int i=4; i<25;i++)  mockIo.setValueForReading(i, 0x0000);
    assertPressedState(true);

    // the first time around, the repeat threshold is 400milli, after that it is 200 millis
    // because we set the repeat interval to 10 of the switch, and the loop delay is 20millis.
    int threshold = 400;

    // make sure we are not already in the held state - we should not be at the moment.
    assertFalse(held);

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
        
        // clear held after each go, the timing of each repeat should be 200.
        held = false;
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

testF(SwitchesFixture, testUpDownEncoder) {
    switches.initialise(&mockIo, true);

    // set up the encoder and add to switches. 1 is up, 2 down.
    EncoderUpDownButtons encoder(1, 2, encoderCallback);
    switches.setEncoder(&encoder);

    // set the range to be 0 to 10
    switches.changeEncoderPrecision(10, 0);

    // do more than 10 up presses
    switches.pushSwitch(1, false);
    for(int i=0; i<20; i++) switches.pushSwitch(1, true);
    
    // the encoder should limit at 10 making only 10 calls.
    assertEqual(encoderCurrentVal, 10);
    assertEqual(callsMade, 11);

    // now do more than 10 down presses.
    switches.pushSwitch(2, false);
    for(int i=0; i<20; i++) switches.pushSwitch(2, true);

    // again the encoder should limit out at another 10 calls (20 in total)
    // and zero reading.
    assertEqual(encoderCurrentVal, 0);
    assertEqual(callsMade, 21);

    // make sure the IO was used correctly
    assertEqual(mockIo.getErrorMode(), NO_ERROR);
}

#define abSet(pinA, pinB) (pinA | (pinB << 1))

testF(SwitchesFixture, testRotaryEncoder) {
    // This is a smoke test, never ever rely on this to exhasutively test the
    // rotary encoder. It just makes sure it has a chance of working by running
    // it through some common cases. A full test is a must if the encoder is changed.
    switches.initialise(&mockIo, true);

    // here we define all the states that the encoder will go through to go up, then down

    // A is 0x01 B is 0x02 - B will be stable when A changes
    // intial state = 0, 1
    mockIo.resetIo();
    mockIo.setValueForReading(0, abSet(0, 1)); // initial state of inputs
    mockIo.setValueForReading(1, abSet(0, 1)); 
    mockIo.setValueForReading(2, abSet(1, 0)); // one click CCY
    mockIo.setValueForReading(3, abSet(0, 0)); 
    mockIo.setValueForReading(4, abSet(1, 1)); // one click CY
    mockIo.setValueForReading(5, abSet(0, 1)); // should be ignored as flicker
    mockIo.setValueForReading(5, abSet(1, 1)); // should be ignored as flicker

    // set up a hardware rotary encoder on pins two and three
    setupRotaryEncoderWithInterrupt(0, 1, encoderCallback);
    assertTrue(mockIo.isIntRegisteredAs(0, CHANGE));

    // and set its range to 10 starting at 5 
    switches.changeEncoderPrecision(10, 5);
    assertEqual(5, encoderCurrentVal);

    runInterruptLoopTimes(2);

    // and check it has gone down.
    assertEqual(encoderCurrentVal, 4);
    assertEqual(callsMade, 2);

    runInterruptLoopTimes(4);

    assertEqual(encoderCurrentVal, 5);
    assertEqual(callsMade, 3);

    assertEqual(mockIo.getErrorMode(), NO_ERROR);
    
    // finally delete the encoder we created earlier.
    delete switches.getEncoder();
}
