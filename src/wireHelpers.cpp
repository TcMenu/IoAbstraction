/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "wireHelpers.h"


uint16_t wireReadReg16(WireType wireType, uint8_t addr, uint8_t reg) {
    ioaWireWriteWithRetry(wireType, addr, &reg, 1, 0, false);

    uint8_t data[2];
    ioaWireRead(wireType, addr, data, sizeof data);
    // read will get port A first then port B.
    uint8_t portA = data[0];
    uint16_t portB = data[1] << 8U;
    return portA | portB;
}

uint8_t wireReadReg8(WireType wireType, uint8_t addr, uint8_t reg) {
    ioaWireWriteWithRetry(wireType, addr, &reg, 1, 0, false);

    uint8_t buffer[2];
    if(ioaWireRead(wireType, addr, buffer, 1)){
        return buffer[0];
    }
    return 0;
}

bool wireWriteReg16(WireType wireType, uint8_t addr, uint8_t reg, uint16_t command) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (uint8_t)command;
    data[2] = (uint8_t)(command>>8);
    return ioaWireWriteWithRetry(wireType, addr, data, sizeof data);
}

bool wireWriteReg8(WireType wireType, uint8_t addr, uint8_t reg, uint8_t command) {
    uint8_t data[2];
    data[0] = reg;
    data[1] = (uint8_t)command;
    return ioaWireWriteWithRetry(wireType, addr, data, sizeof data);
}

void toggleBitInRegister8(WireType wireType, uint8_t addr, uint8_t regAddr, uint8_t theBit, bool value) {
    uint8_t reg = wireReadReg8(wireType, addr, regAddr);
    bitWrite(reg, theBit, value);

    // for debugging to see the commands being sent, uncomment below
    serlogF4(SER_IOA_DEBUG, "toggle(regAddr, bit, toggle): ", regAddr, theBit, value);
    serlogFHex(SER_IOA_DEBUG, "Value: ", reg);
    // end debugging code

    wireWriteReg8(wireType, addr, regAddr, reg);
}

void toggleBitInRegister16(WireType wireType, uint8_t addr, uint8_t regAddr, uint8_t theBit, bool value) {
    uint16_t reg = wireReadReg16(wireType, addr, regAddr);
    bitWrite(reg, theBit, value);

    // for debugging to see the commands being sent, uncomment below
    serlogF4(SER_IOA_DEBUG, "toggle(regAddr, bit, toggle): ", regAddr, theBit, value);
    serlogFHex(SER_IOA_DEBUG, "Value: ", reg);
    // end debugging code

    wireWriteReg16(wireType, addr, regAddr, reg);
}

void write4BitToReg8(WireType wireImpl, uint8_t i2cAddress, uint8_t reg, bool lowBits, uint8_t val) {
    auto regVal = wireReadReg8(wireImpl, i2cAddress, reg);
    if(lowBits) {
        regVal = regVal & 0xF0;
        regVal = regVal | val;
    } else {
        regVal = regVal & 0x0F;
        regVal = regVal | (val << 4);
    }
    wireWriteReg8(wireImpl, i2cAddress, reg, regVal);

}


