
#include <TaskManagerIO.h>
#include <testing/SimpleTest.h>

using namespace SimpleTest;

void setup() {
    Serial.begin(115200);
    //while(!Serial);

    startTesting();
}

void loop() {
    taskManager.runLoop();
}

test(thatBooleanAssertPasses) {
    assertFalse(false);
    assertTrue(true);
}

test(thatBooleanAssertFails) {
    assertFalse(true);
}

test(thatBooleanAssertFails2) {
    assertTrue(false);
}

test(thatIntegerEqualityWorks) {
    assertEquals(10, 10);
    assertLessThan(10, 9);
    assertMoreThan(10, 11);
    assertNotEquals((short)5, (short)10);
    assertEquals('A', 'A');
}

test(testThatAssertLessFails) {
    assertLessThan(10, 20);
}

test(testThatAssertMoreFails) {
    assertLessThan(10, 4);
}

test(testThatAssertEqualFails) {
    assertMoreThan(10, 4);
}

test(testThatAssertNotEqualFails) {
    assertNotEquals(10, 10);
}

test(thatFailWorks) {
    fail("boom");
}

testi(thatIgnoreWorks, true) {
    fail("shouldn't get here");
}

test(stringEqualityWorks) {
    assertStringEquals("hello", "hello");
}
