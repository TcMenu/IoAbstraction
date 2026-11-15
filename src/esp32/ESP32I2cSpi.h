#ifndef ESP32_I2C_WRAPPER_H
#define ESP32_I2C_WRAPPER_H

#include "PlatformDetermination.h"
#include <stddef.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_rom_sys.h"

class Esp32IdfI2CWrapper {
private:
    i2c_port_t port = I2C_NUM_0;
    bool valid = false;
    TickType_t timeoutTicks = pdMS_TO_TICKS(100);
public:
    Esp32IdfI2CWrapper() = default;
    bool begin(gpio_num_t sdaPin, gpio_num_t sclPin, uint32_t frequency = 100000, i2c_port_t i2cPort = I2C_NUM_0, bool enablePullups = true);
    [[nodiscard]] bool isValid() const { return valid; }
    bool wireRead(uint8_t addr, uint8_t *dst, size_t len) const;
    bool wireWrite(uint8_t addr, const uint8_t *src, size_t len, int retries, bool sendStop) const;
};

void ioaWireBegin(pinid_t sdaPin, pinid_t sclPin, uint32_t frequency, i2c_port_t portNum);


#define TC_SPI_WRITE_AVAILABLE

class SPIWithSettings {
private:
    spi_device_handle_t spiBus;
    uint32_t speed;
    pinid_t csPin = 0;
    bool initializedYet = false;
public:
    SPIWithSettings(spi_device_handle_t bus, pinid_t cs) : spiBus(bus), speed(10000000), csPin(cs) {}
    SPIWithSettings(spi_device_handle_t bus, pinid_t cs, uint32_t speed) : spiBus(bus), speed(speed), csPin(cs) {}
    SPIWithSettings(const SPIWithSettings&) = default;
    SPIWithSettings& operator=(const SPIWithSettings&)=default;

    void init() {
        gpio_config_t config = {};
        config.pin_bit_mask = 1ULL << csPin;
        config.mode = GPIO_MODE_OUTPUT;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        config.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&config);
        gpio_set_level(static_cast<gpio_num_t>(csPin), 1);
        initializedYet = true;
    }

    void waitABit() {
        esp_rom_delay_us(1);
    }

    void waitAndActiveCS();

    void waitAndDeactivateCS() {
        if (csPin != 0xFF && csPin != (pinid_t)-1) {
            waitABit();
            gpio_set_level(static_cast<gpio_num_t>(csPin), 1);
            waitABit();
        }
    }

    bool write(const uint8_t* data, size_t size) {
        if (data == nullptr || size == 0) return false;
        waitAndActiveCS();
        spi_transaction_t transaction = {};
        transaction.length = size * 8;
        if (size <= 4) {
            transaction.flags = SPI_TRANS_USE_TXDATA;
            memcpy(transaction.tx_data, data, size);
        } else {
            transaction.tx_buffer = data;
        }
        esp_err_t result = spi_device_polling_transmit(spiBus, &transaction);
        waitAndDeactivateCS();
        return result == ESP_OK;
    }

    bool transferSPI(uint8_t* rdwr, size_t len) {
        if (rdwr == nullptr || len == 0) return false;
        waitAndActiveCS();
        spi_transaction_t transaction = {};
        transaction.length = len * 8;
        if (len <= 4) {
            transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
            memcpy(transaction.tx_data, rdwr, len);
            esp_err_t result = spi_device_polling_transmit(spiBus, &transaction);
            if (result == ESP_OK) {
                memcpy(rdwr, transaction.rx_data, len);
            }
            waitAndDeactivateCS();
            return result == ESP_OK;
        } else {
            transaction.tx_buffer = rdwr;
            transaction.rx_buffer = rdwr;
            esp_err_t result = spi_device_polling_transmit(spiBus, &transaction);
            waitAndDeactivateCS();
            return result == ESP_OK;
        }
    }
};


#endif
