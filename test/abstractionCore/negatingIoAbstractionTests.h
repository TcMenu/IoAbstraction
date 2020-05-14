#line 2 "negatingIoAbstractionTests.h"

#include "MockIoAbstraction.h"
#include "NegatingIoAbstraction.h"


void testNegatingIoAbstractionRead() {
    MockedIoAbstraction mockIo;
    NegatingIoAbstraction negatingIo(&mockIo);

    for(int i=0;i<8;i++) {
        negatingIo.pinDirection(i, INPUT);
        negatingIo.pinDirection(i+8, OUTPUT);
    }

    // set the upfront read values
    mockIo.setValueForReading(0, 0xff);
    mockIo.setValueForReading(1, 0x00);

    // and then check that the read is inverting as expected.
    TEST_ASSERT_TRUE(ioDeviceDigitalRead(&mockIo, 0));
    TEST_ASSERT_FALSE(ioDeviceDigitalRead(&negatingIo, 0));
    // also check the sync is working.
    ioDeviceSync(&negatingIo);
    TEST_ASSERT_FALSE(ioDeviceDigitalRead(&mockIo, 0));
    TEST_ASSERT_TRUE(ioDeviceDigitalRead(&negatingIo, 0));

    // check that port reads are inverted
    TEST_ASSERT_EQUAL(0x00U, (unsigned int)ioDeviceDigitalReadPort(&mockIo, 0));
    TEST_ASSERT_EQUAL(0xffU, (unsigned int)ioDeviceDigitalReadPort(&negatingIo, 0));

    mockIo.resetIo();

    // now test writing to the port is inverted.
    ioDeviceDigitalWrite(&negatingIo, 9, HIGH);
    TEST_ASSERT_EQUAL(mockIo.getWrittenValue(0), (uint16_t)0);
    ioDeviceDigitalWrite(&negatingIo, 9, LOW);
    TEST_ASSERT_EQUAL(mockIo.getWrittenValue(0), (uint16_t)0x200U);

    ioDeviceDigitalWritePort(&negatingIo, 9, 0x00);
    int toCompare = mockIo.getWrittenValue(0) >> 8;
    TEST_ASSERT_EQUAL(toCompare, 0xff);

    // lastly make sure there were no IO errors.
    TEST_ASSERT_EQUAL(mockIo.getErrorMode(), NO_ERROR);
}

