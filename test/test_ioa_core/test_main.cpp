#include <Arduino.h>
#include <unity.h>

void testMockEeprom();
void testMockIoAbstractionRead();
void testMockIoAbstractionWrite();
void testMultiIoPassThrough();
void testNegatingIoAbstractionRead();
void testPressingASingleButton();
void testInterruptButtonRepeating();
void testUpDownEncoder();
void testChangingCallbacks();
void testChangingFromCallbackToListener();
void testChangingFromListenerToCallback();

void setup() {
    Serial.begin(115200);

    UNITY_BEGIN();

    RUN_TEST(testMockEeprom);
    RUN_TEST(testMockIoAbstractionRead);
    RUN_TEST(testMockIoAbstractionWrite);
    RUN_TEST(testMultiIoPassThrough);
    RUN_TEST(testNegatingIoAbstractionRead);
    RUN_TEST(testPressingASingleButton);
    RUN_TEST(testInterruptButtonRepeating);
    RUN_TEST(testPressingASingleButton);
    RUN_TEST(testUpDownEncoder);
    RUN_TEST(testChangingCallbacks);
    RUN_TEST(testChangingFromCallbackToListener);
    RUN_TEST(testChangingFromListenerToCallback);

    UNITY_END();
}

void loop () {

}