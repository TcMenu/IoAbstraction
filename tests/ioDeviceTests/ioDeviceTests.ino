#line 2 "ioDeviceTests.ino"

#include <AUnit.h>
#include "IoAbstractionWire.h"

void setup() {
    Serial.begin(115200);
    while(!Serial);
}

void loop() {
    TestRunner::run();
}

test(test8574ThatsOnlyInput) {
    fail();
}

test(test8574ThatsOnlyOutut) {
    fail();
}

test(test8574PortOperations) {
    fail();
}
