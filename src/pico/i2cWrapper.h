
#ifndef TCCLIBS_I2CWRAPPER_H
#define TCCLIBS_I2CWRAPPER_H

#include "hardware/i2c.h"

class PicoI2cWrapper {
private:
    i2c_inst_t* nativeI2c = nullptr;
public:
    bool isValid() { return nativeI2c != nullptr; }
    void init(i2c_inst_t* i2c) {
        nativeI2c = i2c;
    }

    bool wireRead(uint8_t addr, uint8_t *dst, size_t len);
    bool wireWrite(uint8_t addr, const uint8_t *dst, size_t len, int retries, bool sendStop);
};

#endif //TCCLIBS_I2CWRAPPER_H
