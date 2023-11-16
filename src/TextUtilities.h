/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef IOABSTRACTION_TEXTUTILITIES_H
#define IOABSTRACTION_TEXTUTILITIES_H

#include <PlatformDetermination.h>

/**
 * @file TextUtilities.h
 * @brief A series of text and numeric utilities useful for many purposes.
 */

/**
 * appends a character at the end of the string, if there is space according to len
 */
void appendChar(char* str, char val, int len);

/**
 * used with the below ltoa functions, pass this in padChar when not padded.
 */
#define NOT_PADDED 0

/**
 * A fast long to ascii function that more feature complete than the standard library.
 * Supports zero padding and maximum number of decimal places. This version always
 * starts at position 0 in the provided buffer and goes up to position len always leaving
 * space for a terminator. The other two versions below support appending instead.
 *
 * @param str the buffer to be output to
 * @param val the value to be converted
 * @param divisor the power of 10 largest value (eg 10000, 1000000L etc)
 * @param padChar the character to pad with (or NOT_PADDED which is 0)
 * @param len the length of the buffer passed in, it will not be exceeded.
 */
void ltoaClrBuff(char* str, long val, uint8_t dp, char padChar, int len);

/**
 * A fast long to ascii function that more feature complete than the standard library.
 * Supports zero padding and the largest actual value to use normally a power of 10.
 * Absolute largest value displayable is 1000000000 - 1. NOTE that this function will
 * append at the end of the current string. Use ltoaClrBuff to start at position 0.
 * This call will not exceed the length provided and will properly terminate the string.
 *
 * @param str the buffer to be appended to
 * @param val the value to be converted
 * @param divisor the power of 10 largest value (eg 10000, 1000000L etc)
 * @param padChar the character to pad with (or NOT_PADDED which is 0)
 * @param len the length of the buffer passed in, it will not be exceeded.
 */
void fastltoa_mv(char* str, long val, long divisor, char padChar, int len);

/**
 * A fast long to ascii function that more feature complete than the standard library.
 * Supports zero padding and the number of decimal places to use. Maximum number of
 * decimal places is 9. NOTE that this function will append at the end of the current
 * string and will not exceed the length provided, it will also properly terminate the
 * string. Use ltoaClrBuff to start at position 0 in the buffer.
 *
 * @param str the buffer to be appended to
 * @param val the value to be converted
 * @param dp the number of decimal places allowed
 * @param padChar the character to pad with (or NOT_PADDED which is 0)
 * @param len the length of the buffer passed in, it will not be exceeded.
 */
void fastltoa(char* str, long val, uint8_t dp, char padChar, int len);

/**
 * A very simple floating point string function based on the fastltoa above. It can print
 * floating point values up to 9 whole digits and 9 decimal places. Note this function
 * appends the floating point value at the end of the string, to put the value at the
 * beginning, ensure the string is zero length.
 * @param sz the string to append to,
 * @param fl the float to convert
 * @param dp the numer of decimal places (max 9)
 * @param strSize the string maximum length (usually from sizeof)
 */
void fastftoa(char* sz, float fl, int dp, int strSize);

/**
 * converts decimal places into a suitable divisor, eg: 2 -> 100, 4 -> 10000
 */
long dpToDivisor(int dp);

/**
 * Indicates how many integers are needed to represent the value and negative flag if provided
 * @param value the value to represent as unsigned.
 * @param negative if the value is negative
 * @return the number of characters including the sign needed
 */
long valueToSignificantPlaces(unsigned long value, bool negative);

/**
 * Get the hex equivalent of a single digit, input must be between 0 and 15
 * @param val the input unsigned integer between 0 and 15
 * @return a hex character from 0..F
 */
char hexChar(uint8_t val);

/**
 * Get the integer representation of a hex digit, between 0..F
 * @param val the input character
 * @return the numeric representation.
 */
uint8_t hexValueOf(char val);

/**
 * Get the hex string for a number to a given number of fixed places, EG 4 digits. You can optionally include the
 * '0x' at the beginning.
 * @param buffer the buffer to copy into
 * @param bufferSize the size of the buffer
 * @param input the input number
 * @param digits the number of FIXED digits
 * @param with0x if 0x should be included at the beginning
 */
void intToHexString(char* buffer, size_t bufferSize, unsigned int input, int digits, bool with0x);

/**
 * Ensure that a float is always positive.
 * @param f1 the float that could be positive or negative
 * @return the positive version of the float.
 */
inline float tcFltAbs(float f1) {
    return f1 > 0.0F ? f1 : -f1;
}

#if defined(IOA_USE_MBED) || defined(BUILD_FOR_PICO_CMAKE)
#define strcmp_P(x,y) strcmp(x,y)
#define strncpy_P(x,y,z) strncpy(x,y,z)
#define strcpy_P(x,y) strcpy(x,y)
#define strlen_P(x) strlen(x)
#define highByte(x) ((x) >> 8)
#define lowByte(x) ((x) & 0xff)
#define ltoa(a,b,c) itoa(a,b,c)
# ifndef min
# define min(x, y) (((x) < (y))?(x):(y))
# define max(x, y) (((x) > (y))?(x):(y))
# endif //TCMENU_MBED_NO_MINMAX
#endif // IOA_USE_MBED

#ifdef IOA_ARDUINO_MBED
#define ltoa(a,b,c) itoa(a,b,c)
#endif

#endif //IOABSTRACTION_TEXTUTILITIES_H
