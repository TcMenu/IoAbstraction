#include <TaskManagerIO.h>
#include <unity.h>
#include "MockIoAbstraction.h"
#include "SwitchInput.h"

bool pressed;
bool keyReleased;
uint8_t key;
bool held;
int callsMade;
int callsMade2;
int encoderCurrentVal;

// this allows us to call the switches interrupt handler
extern void onSwitchesInterrupt(uint8_t);

void encoderCallback(int newValue) {
    encoderCurrentVal = newValue;
    callsMade++;
}

void encoderCallback2(int newVal) {
    encoderCurrentVal = newVal;
    callsMade2++;
}

void onSwitchPressed(pinid_t k, bool h) {
    callsMade++;
    key = k;
    held = h;
    pressed = true;
}

void onSwitchReleased(pinid_t, bool h) {
    keyReleased = true;
    held = h;
}

class SwitchesFixture {
public:
    MockedIoAbstraction mockIo;
    SwitchesFixture() : mockIo(25) { }

    void setup() {
        mockIo.resetIo();
        taskManager.reset();

        pressed = false;
        keyReleased = false;
        held = false;
        callsMade = 0;
        callsMade2 = 0;
        encoderCurrentVal = 0;
    }

    void teardown() {
        switches.setEncoder(0, nullptr);
        switches.resetAllSwitches();
        mockIo.resetIo();
        taskManager.reset();
        switches.setEncoder(nullptr);
    }

    void assertPressedState(bool shouldBePressed) {
        // wait until the callback fires or we time out
        int loopCount = 0;
        while(pressed != shouldBePressed && ++loopCount < 200) {
            if(switches.isInterruptDriven()) mockIo.getInterruptFunction()();
            taskManager.yieldForMicros(2000);
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
};

uint32_t safeMilliDiffFromNow(unsigned long then) {
    unsigned long now = millis();
    if(then > now) return 0;
    return now - then;
}

void testPressingASingleButton() {
    SwitchesFixture fixture;
    fixture.setup();
    switches.initialise(&fixture.mockIo, true);
    switches.addSwitch(2, onSwitchPressed, NO_REPEAT);
    switches.onRelease(2, onSwitchReleased);
    TEST_ASSERT_FALSE(switches.isInterruptDriven());

    // simulate the button being pressed (with a bounce).
    fixture.mockIo.setValueForReading(0, 0x0000); // pressed
    fixture.mockIo.setValueForReading(1, 0x0000); // pressed
    TEST_ASSERT_FALSE(pressed);
    fixture.mockIo.setValueForReading(2, 0x0004); // bounce

    // we now simulate the button being pressed down.
    for(int i=3; i<25;i++)  fixture.mockIo.setValueForReading(i, 0x0000);
    fixture.assertPressedState(true);
    TEST_ASSERT_FALSE(keyReleased);

    auto millisStart = millis();

    // we now simulate the button being held, long press
    fixture.mockIo.resetIo();
    for(int i=0; i<25;i++)  fixture.mockIo.setValueForReading(i, 0x0000);
    fixture.assertHeldState(true);

    // we now simulate the button being released.
    fixture.mockIo.resetIo();
    for(int i=0; i<25;i++)  fixture.mockIo.setValueForReading(i, 0x0004);
    fixture.assertReleasedState(true);

    // make sure getting to held took near to 400 millis.
    TEST_ASSERT_GREATER_THAN((uint32_t)380, safeMilliDiffFromNow(millisStart));
    TEST_ASSERT_LESS_THAN((uint32_t)450, safeMilliDiffFromNow(millisStart));
    fixture.teardown();
}

void testInterruptButtonRepeating() {
    SwitchesFixture fixture;
    fixture.setup();

    // initialise the switches library using interrupt based initialisation.
    switches.initialiseInterrupt(&fixture.mockIo, true);
    switches.addSwitch(2, onSwitchPressed, 10);
    TEST_ASSERT_TRUE(fixture.mockIo.isIntRegisteredAs(2, CHANGE));
    TEST_ASSERT_TRUE(switches.isInterruptDriven());

    // simulate the button being pressed (with a double bounce - debounce 2).
    fixture.mockIo.setValueForReading(0, 0x0000);
    fixture.mockIo.setValueForReading(1, 0x0000);
    fixture.mockIo.setValueForReading(2, 0x0004);
    fixture.mockIo.setValueForReading(3, 0x0000);
    fixture.mockIo.setValueForReading(4, 0x0004);

    // we now simulate the button being pressed down (and the interrupt handler).
    for(int i=4; i<25;i++)  fixture.mockIo.setValueForReading(i, 0x0000);
    fixture.assertPressedState(true);

    // the first time around, the repeat threshold is 400milli, after that it is 200 millis
    // because we set the repeat interval to 10 of the switch, and the loop delay is 20millis.
    int threshold = 500;

    // make sure we are not already in the held state - we should not be at the moment.
    TEST_ASSERT_FALSE(held);

    // try a few repeat keys..
    for(int i=0; i<4; i++) {
        // capture when we started
        auto timeThen = millis();

        // we now simulate the button being held for a while
        fixture.mockIo.resetIo();
        for(int j=0; j<25;j++)  fixture.mockIo.setValueForReading(j, 0x0000);
        fixture.assertHeldState(true);

        // ensure it is repeating - IE the calls made should be near to the count.
        TEST_ASSERT_GREATER_THAN(i - 1, callsMade);

        // clear held after each go, the timing of each repeat should be 200.
        held = false;
    }

    // keep track of the number of calls before button release
    int callsWhenButtonReleased = callsMade;

    // now simulate letting go of the button on the IO device.
    fixture.mockIo.resetIo();
    for(int j=0; j<25;j++)  fixture.mockIo.setValueForReading(j, 0x0004);
    fixture.mockIo.getInterruptFunction()();

    // wait long enough for switches to have triggered at least once.
    taskManager.yieldForMicros(32000);

    // we should not have received any more events.
    TEST_ASSERT_EQUAL(callsMade, callsWhenButtonReleased);
    fixture.teardown();
}


void testUpDownEncoder() {
    SwitchesFixture fixture;
    fixture.setup();
    switches.initialise(&fixture.mockIo, true);
    EncoderUpDownButtons upDownEncoder(1, 2, encoderCallback);

    // set up the encoder and add to switches. 1 is up, 2 down.
    switches.setEncoder(0, &upDownEncoder);

    // set the range to be 0 to 10
    switches.changeEncoderPrecision(10, 0);

    // do more than 10 up presses
    switches.pushSwitch(1, false);
    for(int i=0; i<20; i++) switches.pushSwitch(1, true);

    // the encoder should limit at 10 making only 10 calls.
    TEST_ASSERT_EQUAL(encoderCurrentVal, 10);

    // now do more than 10 down presses.
    switches.pushSwitch(2, false);
    for(int i=0; i<20; i++) switches.pushSwitch(2, true);

    // again the encoder should limit out at another 10 calls (20 in total)
    // and zero reading.
    TEST_ASSERT_EQUAL(encoderCurrentVal, 0);

    // make sure the IO was used correctly
    TEST_ASSERT_EQUAL(fixture.mockIo.getErrorMode(), NO_ERROR);
    fixture.teardown();
}

void testChangingCallbacks() {
    SwitchesFixture fixture;
    fixture.setup();
    switches.initialise(&fixture.mockIo, true);
    switches.addSwitch(6, onSwitchPressed, NO_REPEAT);
    switches.pushSwitch(6, false);

    TEST_ASSERT_EQUAL(0, callsMade2);
    TEST_ASSERT_TRUE(callsMade != 0);
    auto oldNumberOfCalls = callsMade;

    switches.replaceOnPressed(6, [](pinid_t pin, bool held) {
        callsMade2++;
    });
    switches.pushSwitch(6, false);
    TEST_ASSERT_TRUE(callsMade2 != 0);
    TEST_ASSERT_EQUAL(callsMade, oldNumberOfCalls);
    switches.setEncoder(0, nullptr);
    fixture.teardown();
}

class MyTestSwitchListener : public SwitchListener {
private:
    int numPressed;
    int numReleased;
public:
    MyTestSwitchListener() : numPressed(0), numReleased(0) {
    }

    void reset() {
        numPressed = numReleased = 0;
    }

    void onPressed(pinid_t pin, bool h) override {
        numPressed++;
    }

    void onReleased(pinid_t pin, bool h) override {
        numReleased++;
    }

    bool wasActivated() { return numPressed !=0 || numReleased != 0; }
} testSwitchListener;

void testChangingFromCallbackToListener() {
    SwitchesFixture fixture;
    fixture.setup();
    switches.initialise(&fixture.mockIo, true);
    testSwitchListener.reset();
    switches.addSwitch(6, onSwitchPressed, NO_REPEAT);
    switches.replaceSwitchListener(6, &testSwitchListener);
    switches.pushSwitch(6, false);
    TEST_ASSERT_EQUAL(callsMade, 0);
    TEST_ASSERT_TRUE(testSwitchListener.wasActivated());
    fixture.teardown();
}

void testChangingFromListenerToCallback() {
    SwitchesFixture fixture;
    fixture.setup();
    switches.initialise(&fixture.mockIo, true);
    testSwitchListener.reset();
    switches.addSwitchListener(6, &testSwitchListener, NO_REPEAT);
    switches.replaceOnPressed(6, onSwitchPressed);
    switches.pushSwitch(6, false);
    TEST_ASSERT_EQUAL(callsMade, 1);
    TEST_ASSERT_FALSE(testSwitchListener.wasActivated());
    fixture.teardown();
}
