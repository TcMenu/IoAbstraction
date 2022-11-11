
#include <TaskManagerIO.h>
#include <testing/SimpleTest.h>

using namespace SimpleTest;

IOLOG_MBED_PORT_IF_NEEDED(USBTX, USBRX)

void setup() {
    IOLOG_START_SERIAL
    startTesting();
}

DEFAULT_TEST_RUNLOOP

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
    assertEquals(10, 4);
}

test(testThatAssertNotEqualFails) {
    assertNotEquals(10, 10);
}

test(testThatFloatWithinRangeOk) {
    assertFloatNear(100.002, 100.001, 0.01);
}

test(testThatFloatOutsideRangeFails) {
    assertFloatNear(100.002, 10.001, 0.01);
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

test(hitsOnlyFirstAssertion) {
    fail("1");
    fail("2");
    assertTrue(false);
    assertEquals(10, 4);
    assertFloatNear(10.4, 4.2, 0.01);
    assertStringEquals("123", "432");
}