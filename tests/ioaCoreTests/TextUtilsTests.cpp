
#include <testing/SimpleTest.h>
#include <TextUtilities.h>

test(testTcUtilIntegerConversions) {
        char szBuffer[20];

        // first check the basic cases for the version that always starts at pos 0
        strcpy(szBuffer, "abc");
        ltoaClrBuff(szBuffer, 1234, 4, ' ', sizeof(szBuffer));
        assertStringEquals(szBuffer, "1234");
        ltoaClrBuff(szBuffer, 907, 4, ' ', sizeof(szBuffer));
        assertStringEquals(szBuffer, " 907");
        ltoaClrBuff(szBuffer, 22, 4, '0', sizeof(szBuffer));
        assertStringEquals(szBuffer, "0022");
        ltoaClrBuff(szBuffer, -22, 4, '0', sizeof(szBuffer));
        assertStringEquals(szBuffer, "-0022");
        ltoaClrBuff(szBuffer, -93, 2, NOT_PADDED, sizeof(szBuffer));
        assertStringEquals(szBuffer, "-93");
        ltoaClrBuff(szBuffer, 0, 4, NOT_PADDED, sizeof(szBuffer));
        assertStringEquals(szBuffer, "0");

        // and now test the appending version with zero padding
        strcpy(szBuffer, "val = ");
        fastltoa(szBuffer, 22, 4, '0', sizeof(szBuffer));
        assertStringEquals(szBuffer, "val = 0022");

        // and now test the appending version with an absolute divisor.
        strcpy(szBuffer, "val = ");
        fastltoa_mv(szBuffer, 22, 1000, '0', sizeof(szBuffer));
        assertStringEquals(szBuffer, "val = 022");

        // and lasty try the divisor version without 0.
        strcpy(szBuffer, "val = ");
        fastltoa_mv(szBuffer, 22, 10000, NOT_PADDED, sizeof(szBuffer));
        assertStringEquals(szBuffer, "val = 22");

        // and now try something bigger than the divisor
        strcpy(szBuffer, "val = ");
        fastltoa_mv(szBuffer, 222222, 10000, NOT_PADDED, sizeof(szBuffer));
        assertStringEquals(szBuffer, "val = 2222");
}

test(testTcUtilHexCoversions) {
    char szBuffer[20];
    assertEquals('0', hexChar(0));
    assertEquals('9', hexChar(9));
    assertEquals('A', hexChar(10));
    assertEquals('F', hexChar(15));

    intToHexString(szBuffer, sizeof szBuffer, 0xfade, 4, true);
    assertStringEquals("0xFADE", szBuffer);

    intToHexString(szBuffer, sizeof szBuffer, 0x0000, 4, true);
    assertStringEquals("0x0000", szBuffer);

    intToHexString(szBuffer, 6, 0xFFFF, 4, true);
    assertStringEquals("0xFFF", szBuffer);

    intToHexString(szBuffer, 3, 0xFFFF, 4, false);
    assertStringEquals("FF", szBuffer);
}