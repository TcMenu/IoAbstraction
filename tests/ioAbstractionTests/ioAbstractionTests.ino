#include <AUnit.h>
//#include <AUnitVerbose.h>

using namespace aunit;

#include "eepromTests.h"
#include "ioDeviceTests.h"
#include "negatingIoAbstractionTests.h"
#include "switchesTests.h"
#include "taskManagerTests.h"

// always run this last as it adjusts the millisecond counter, to test rolling.
#ifdef __AVR__
#include "avrTaskManagerClockRoll.h"
#endif 

void setup() {
	Serial.begin(115200);
	while(!Serial);
	Wire.begin();

	TestRunner::setTimeout(60); // max wait 60 seconds.

    // should you wish to run less that the full suite, here are example excludes / includes.
	//TestRunner::exclude("HighThroughputFixture_taskManagerHighThroughputTest");
	//TestRunner::include("TimingHelpFixture_cancellingAJobAfterCreation");
}

void loop() {
	TestRunner::run();
}
