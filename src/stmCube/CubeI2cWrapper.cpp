
#ifdef BUILD_FOR_STM32CUBE_CMAKE
#include "CubeI2cWrapper.h"
bool CubeI2cWrapper::wireRead(uint8_t addr, uint8_t *dst, size_t len) const {
    return HAL_I2C_Master_Receive(i2cHandle, addr, dst, len, TC_TIMEOUT_UINT_MS) == HAL_OK;
}

bool CubeI2cWrapper::wireWrite(uint8_t addr, const uint8_t *dst, size_t len, int retries, bool sendStop) const {
    return HAL_I2C_Master_Transmit(i2cHandle, addr, const_cast<uint8_t*>(dst), len, TC_TIMEOUT_UINT_MS) == HAL_OK;
}

#endif
