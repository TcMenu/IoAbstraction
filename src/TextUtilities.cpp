/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <TaskPlatformDeps.h>
#include "TextUtilities.h"

void appendChar(char* str, char val, int len) {
    int i = 0;
    len -= 2;
    while(str[i] && len) {
        --len;
        ++i;
    }
    str[i++] = val;
    str[i] = (char)0;
}

long dpToDivisor(int dp) {
    switch(dp) {
        case 8: return 100000000L;
        case 7: return 10000000L;
        case 6: return 1000000L;
        case 5: return 100000;
        case 4: return 10000;
        case 3: return 1000;
        case 2: return 100;
        case 1: return 10;
        case 0: return 1;
        default:
        case 9: return 1000000000L;
    }
}

long valueToSignificantPlaces(unsigned long value, bool negative) {
    unsigned long divisor = 10U;
    int places = 1;
    while(value > divisor) {
        divisor *= 10U;
        places = places + 1;
    }
    return negative ? (places + 1) : places;
}

void ltoaClrBuff(char* str, long val, uint8_t dp, char padChar, int len) {
    str[0]=0;
    fastltoa_mv(str, val, dpToDivisor(dp), padChar, len);
}

void fastltoa(char* str, long val, uint8_t dp, char padChar, int len) {
    fastltoa_mv(str, val, dpToDivisor(dp), padChar, len);
}

void fastltoa_mv(char* str, long val, long divisor, char padChar, int len) {
    int i=0;
    len -=2;

    if (val < 0) {
        val = abs(val);
        appendChar(str, '-', len);
    }

    val %= divisor;
    divisor /= 10;

    while(str[i] && i < len) ++i;

    bool hadNonZeroChar = false;
    bool zeroPad = padChar != 0;

    while(divisor > 9 && i < len) {
        str[i] = (char)((val / divisor) + '0');
        hadNonZeroChar |= (str[i] != '0');
        if(zeroPad && !hadNonZeroChar) str[i] = padChar;
        if(zeroPad || hadNonZeroChar) ++i;
        val %= divisor;
        divisor /= 10;
    }
    str[i++] = '0' + (val % 10);
    str[i] = (char)0;
}

void fastftoa(char* sz, float fl, int dp, int strSize) {
    bool neg = false;
    if(fl < 0.0F) {
        fl = fl * -1.0F;
        neg = true;
    }

    // here we get the whole and fractonal parts, knowing its always positive, lastly, we
    // multiply it up by decimal places to turn it into an int, then we can present it as "[-]whole.fraction"
    auto whole = (int32_t)fl;
    fl = fl - float(whole);
    auto fraction = int32_t(fl * (float)dpToDivisor(dp));

    if(neg) appendChar(sz, '-', strSize);
    fastltoa(sz, whole, 9, NOT_PADDED, strSize);
    appendChar(sz, '.', strSize);
    fastltoa(sz, fraction, dp, '0', strSize);
}

char hexChar(uint8_t val) {
    if(val < 10) return char(val + '0');
    return char(val - 10) + 'A';
}


void intToHexString(char* buffer, size_t bufferSize, unsigned int input, int digits, bool with0x) {
    if(with0x) {
        buffer[0] = '0';
        buffer[1] = 'x';
        bufferSize -= 2;
        buffer += 2;
    }

    if(digits >= bufferSize) {
        digits = bufferSize - 1;
    }

    int i = 0;
    while(bufferSize && i < digits) {
        buffer[(digits-1) - i] = hexChar(input & 0x0f);
        input = input >> 4;
        i++;
    }
    buffer[i] = 0;
}

uint8_t hexValueOf(char val) {
    if(val >= '0' && val <= '9') return val - '0';
    val = (char)toupper(val);
    if(val >= 'A' && val <= 'F') return val - ('A' - 10);
    return 0;
}