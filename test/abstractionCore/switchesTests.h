#include "MockIoAbstraction.h"
#include "SwitchInput.h"
#include <unity.h>

bool pressed;
bool keyReleased;
uint8_t key;
bool held;
int callsMade;
int encoderCurrentVal;
MockedIoAbstraction mockIo(25);

// this allows us to call the switches interrupt handler
extern void onSwitchesInterrupt(uint8_t);

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


void setupTest() {
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
    TEST_ASSERT_EQUAL(pressed, shouldBePressed);
    TEST_ASSERT_FALSE(keyReleased);
    TEST_ASSERT_EQUAL(mockIo.getErrorMode(), NO_ERROR);
}

void assertHeldState(bool shouldBeHeld) {
    int loopCount = 0;
    while(held != shouldBeHeld && ++loopCount < 500) {
        taskManager.yieldForMicros(1000);
    }

    // now check if we are held and for any errors
    TEST_ASSERT_EQUAL(held, shouldBeHeld);
    TEST_ASSERT_FALSE(keyReleased);
    TEST_ASSERT_EQUAL(mockIo.getErrorMode(), NO_ERROR);
}

void assertReleasedState(bool expectedHeldState) {
    int loopCount = 0;
    while(!keyReleased && ++loopCount < 500) {
        taskManager.yieldForMicros(1000);
    }
    TEST_ASSERT_TRUE(keyReleased);
    TEST_ASSERT_EQUAL(held, expectedHeldState);
}

void runInterruptLoopTimes(int times) {
    for(int i=0; i<times; i++) {
        mockIo.getInterruptFunction()();
        taskManager.yieldForMicros(10);
    }
}


void testPressingASingleButton() {
    setupTest();

    switches.initialise(&mockIo, true);
    switches.addSwitch(2, onSwitchPressed, NO_REPEAT);
    switches.onRelease(2, onSwitchReleased);
    TEST_ASSERT_FALSE(switches.isInterruptDriven());

    // simulate the button being pressed (with a bounce).
    mockIo.setValueForReading(0, 0x0000); // pressed
    mockIo.setValueForReading(1, 0x0000); // pressed
    TEST_ASSERT_FALSE(pressed);
    mockIo.setValueForReading(2, 0x0004); // bounce 

    // we now simulate the button being pressed down.
    for(int i=3; i<25;i++)  mockIo.setValueForReading(i, 0x0000);
    assertPressedState(true);
    TEST_ASSERT_FALSE(keyReleased);

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
    TEST_ASSERT_GREATER_OR_EQUAL((uint32_t)380, millis() - millisStart);
    TEST_ASSERT_LESS_OR_EQUAL((uint32_t)450, millis() - millisStart);
}

void testInterruptButtonRepeating() {
    setupTest();

    // initialise the switches library using interrupt based initialisation.
    switches.initialiseInterrupt(&mockIo, true);
    switches.addSwitch(2, onSwitchPressed, 10);
    TEST_ASSERT_TRUE(mockIo.isIntRegisteredAs(2, CHANGE));
    TEST_ASSERT_TRUE(switches.isInterruptDriven());

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
    TEST_ASSERT_FALSE(held);

    // try a few repeat keys..
    for(int i=0; i<4; i++) {
        // capture when we started
        long timeThen = millis();

        // we now simualte the button being held for a while
        mockIo.resetIo();
        for(int j=0; j<25;j++)  mockIo.setValueForReading(j, 0x0000);
        assertHeldState(true);

        // ensure it is repeating - IE the calls made should be near to the count.
        TEST_ASSERT_GREATER_OR_EQUAL(i, callsMade);

        // ensure the timing is close to what we expect.
        TEST_ASSERT_GREATER_OR_EQUAL((uint32_t)threshold  - 50, millis() - timeThen);
        TEST_ASSERT_LESS_OR_EQUAL((uint32_t)threshold + 50, millis() - timeThen);
        
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
    TEST_ASSERT_EQUAL(callsMade, callsWhenButtonReleased);
}

void testUpDownEncoder() {
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
    TEST_ASSERT_EQUAL(encoderCurrentVal, 10);
    TEST_ASSERT_GREATER_OR_EQUAL(10, callsMade);

    // now do more than 10 down presses.
    switches.pushSwitch(2, false);
    for(int i=0; i<20; i++) switches.pushSwitch(2, true);

    // again the encoder should limit out at another 10 calls (20 in total)
    // and zero reading.
    TEST_ASSERT_EQUAL(encoderCurrentVal, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(20, callsMade);

    // make sure the IO was used correctly
    TEST_ASSERT_EQUAL(mockIo.getErrorMode(), NO_ERROR);
}
