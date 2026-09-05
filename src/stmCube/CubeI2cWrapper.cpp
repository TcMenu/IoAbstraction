
#ifdef BUILD_FOR_STM32CUBE_CMAKE
#include "CubeI2cWrapper.h"
#ifndef IO_DISABLE_STMI2C
bool CubeI2cWrapper::wireRead(uint8_t addr, uint8_t *dst, size_t len) const {
    return HAL_I2C_Master_Receive(i2cHandle, addr, dst, len, TC_TIMEOUT_UINT_MS) == HAL_OK;
}

bool CubeI2cWrapper::wireWrite(uint8_t addr, const uint8_t *dst, size_t len, int retries, bool sendStop) const {
    return HAL_I2C_Master_Transmit(i2cHandle, addr, const_cast<uint8_t*>(dst), len, TC_TIMEOUT_UINT_MS) == HAL_OK;
}
#endif

#ifndef IO_DISABLE_STMSPI

void SPIWithSettings::waitAndActiveCS() {
    int retries = 50;
    while(HAL_SPI_GetState(spiBus) != HAL_SPI_STATE_READY && retries > 0) {
        retries--;
        serlogF2(SER_IOA_DEBUG, "SPI busy retries=", retries);
    }

    internalDigitalDevice().digitalWrite(csPin, LOW);
    waitABit();
}

#endif

#endif
