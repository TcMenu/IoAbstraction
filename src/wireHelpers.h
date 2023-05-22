/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef IOA_WIRE_HELPERS_H
#define IOA_WIRE_HELPERS_H

#include "IoAbstraction.h"
#include "PlatformDetermination.h"
#include "PlatformDeterminationWire.h"
#include "IoLogging.h"

/**
 * @file wireHelpers.h
 * @brief a series of wire helper functions that make dealing with device register IO simpler
 */


/**
 * Writes a 4 bit value into an 8 bit register preserving the other 4 bits.
 * @param wireImpl the wire implementation to use.
 * @param i2cAddress the i2c address
 * @param reg the register to write to
 * @param lowBits if the low or high bits are to be modified
 * @param val the new value between 0..15
 */
void write4BitToReg8(WireType wireImpl, uint8_t i2cAddress, uint8_t reg, bool lowBits, uint8_t val);

/**
 * Toggles a bit in a 16 bit register to the new value
 * @param wireType the wire implementation to use
 * @param addr the i2c address
 * @param regAddr the register to write to
 * @param theBit the bit to change
 * @param value the new value
 */
void toggleBitInRegister16(WireType wireType, uint8_t addr, uint8_t regAddr, uint8_t theBit, bool value);

/**
 * Toggles a bit in an 8 bit register to the new value
 * @param wireType the wire implementation to use
 * @param addr the i2c address
 * @param regAddr the register to write to
 * @param theBit the bit to change
 * @param value the new value
 */
void toggleBitInRegister8(WireType wireType, uint8_t addr, uint8_t regAddr, uint8_t theBit, bool value);

/**
 * Writes an 8 bit value to a register
 * @param wireType the wire implementation
 * @param addr the i2c address
 * @param reg the register to change
 * @param command the value for the register
 * @return true if success
 */
bool wireWriteReg8(WireType wireType, uint8_t addr, uint8_t reg, uint8_t command);

/**
 * Writes a 16 bit value to a register
 * @param wireType the wire implementation
 * @param addr the i2c address
 * @param reg the register to change
 * @param command the value for the register
 * @return true if success
 */
bool wireWriteReg16(WireType wireType, uint8_t addr, uint8_t reg, uint16_t command);

/**
 * Reads an 8 bit value from a given register
 * @param wireType the wire implementation
 * @param addr the i2c address
 * @param reg the register to read
 * @return the register value
 */
uint8_t wireReadReg8(WireType wireType, uint8_t addr, uint8_t reg);

/**
 * Reads a 16 bit value from a given register
 * @param wireType the wire implementation
 * @param addr the i2c address
 * @param reg the register to read
 * @return the register value
 */
uint16_t wireReadReg16(WireType wireType, uint8_t addr, uint8_t reg);

#endif // IOA_WIRE_HELPERS_H
