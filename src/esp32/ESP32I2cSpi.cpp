#include "ESP32I2cSpi.h"
#include "../extras/SPIHelper.h"

#include "PlatformDeterminationWire.h"

#if defined(BUILD_FOR_ESP32_IDF)



#endif
bool Esp32IdfI2CWrapper::begin(gpio_num_t sdaPin, gpio_num_t sclPin, uint32_t frequency, i2c_port_t i2cPort, bool enablePullups) {
    port = i2cPort;

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sdaPin;
    conf.scl_io_num = sclPin;
    conf.sda_pullup_en = enablePullups ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = enablePullups ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = frequency;
    conf.clk_flags = 0;

    esp_err_t err = i2c_param_config(port, &conf);
    if (err != ESP_OK) {
        valid = false;
        return false;
    }

    err = i2c_driver_install(port, conf.mode, 0, 0, 0);
    valid = (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
    return valid;
}

bool Esp32IdfI2CWrapper::wireRead(uint8_t addr, uint8_t *dst, size_t len) const {
    if (!valid || dst == nullptr || len == 0) {
        return false;
    }

    const esp_err_t err = i2c_master_read_from_device(
        port,
        addr,
        dst,
        len,
        timeoutTicks
    );

    return err == ESP_OK;

}

bool Esp32IdfI2CWrapper::wireWrite(uint8_t addr, const uint8_t *src, size_t len, int retries, bool sendStop) const {
    if (!valid || src == nullptr || len == 0) {
        return false;
    }

    if (retries < 1) {
        retries = 1;
    }

    for (int attempt = 0; attempt < retries; ++attempt) {
        esp_err_t err;

        if (sendStop) {
            err = i2c_master_write_to_device(
                port,
                addr,
                src,
                len,
                timeoutTicks
            );
        } else {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            if (cmd == nullptr) {
                return false;
            }

            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, static_cast<uint8_t>((addr << 1) | I2C_MASTER_WRITE), true);
            i2c_master_write(cmd, src, len, true);

            err = i2c_master_cmd_begin(port, cmd, timeoutTicks);
            i2c_cmd_link_delete(cmd);
        }

        if (err == ESP_OK) {
            return true;
        }

        vTaskDelay(1);
    }

    return false;
}

void ioaWireBegin(pinid_t sdaPin, pinid_t sclPin, uint32_t frequency, i2c_port_t portNum) {
    defaultWireTypePtr->begin(static_cast<gpio_num_t>(sdaPin), static_cast<gpio_num_t>(sclPin), frequency, portNum);
}

bool ioaWireRead(WireType wire, int address, uint8_t* buffer, size_t len) {
    if(wire == nullptr || !wire->isValid()) return false;
    return wire->wireRead(address, buffer, len);
}

bool ioaWireWriteWithRetry(WireType wire, int address, const uint8_t* buffer, size_t len, int retriesAllowed, bool sendStop) {
    if(wire == nullptr || !wire->isValid()) return false;
    return wire->wireWrite(address, buffer, len, retriesAllowed, sendStop);
}

void SPIWithSettings::waitAndActiveCS() {
    if(!initializedYet) {
        init();
    }
    gpio_set_level(static_cast<gpio_num_t>(csPin), 0);
    waitABit();
}
