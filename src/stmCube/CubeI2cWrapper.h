
#ifndef IO_ABSTRACTION_CUBEI2CWRAPPER_H
#define IO_ABSTRACTION_CUBEI2CWRAPPER_H

#include <TaskPlatformDeps.h>
#include <IoAbstraction.h>
#define TC_TIMEOUT_UINT_MS 1000

#ifndef IO_DISABLE_STMI2C

class CubeI2cWrapper {
private:
    I2C_HandleTypeDef* i2cHandle = nullptr;
public:
    [[nodiscard]] bool isValid() const { return i2cHandle != nullptr; }
    void init(I2C_HandleTypeDef* i2c) {
        i2cHandle = i2c;
    }
    bool wireRead(uint8_t addr, uint8_t *dst, size_t len) const;
    bool wireWrite(uint8_t addr, const uint8_t *dst, size_t len, int retries, bool sendStop) const;
};

typedef CubeI2cWrapper* WireType;
void ioaWireBegin(I2C_HandleTypeDef* handleI2c);
#else

typedef void* WireType;
void ioaWireBegin(WireType* handleI2c);

#endif

#ifndef IO_DISABLE_STMSPI

class SPIWithSettings {
private:
    SPI_HandleTypeDef* spiBus;
    uint32_t speed;
    pinid_t csPin;
public:
    SPIWithSettings(SPI_HandleTypeDef* bus, const pinid_t cs) : spiBus(bus), speed(10000000), csPin(cs) {}
    SPIWithSettings(SPI_HandleTypeDef* bus, const pinid_t cs, uint32_t speed) : spiBus(bus), speed(speed), csPin(cs) {}
    SPIWithSettings(const SPIWithSettings&) = default;
    SPIWithSettings& operator=(const SPIWithSettings&) = default;

    void init() {}

    void waitABit() {
        asm volatile("nop \n nop \n nop");
    }

    void waitAndActiveCS();

    void waitAndDeactivateCS() {
        waitABit();
        internalDigitalDevice().digitalWrite(csPin, HIGH);
        waitABit();
    }

    bool write(const uint8_t* data, const size_t size) {
        waitAndActiveCS();
        int written = HAL_SPI_Transmit(spiBus, data, size, TC_TIMEOUT_UINT_MS);
        waitAndDeactivateCS();
        return written == size;
    }

    bool transferSPI(uint8_t* rdwr, const size_t len) {
        waitAndActiveCS();
        if (HAL_OK != HAL_SPI_TransmitReceive(spiBus, rdwr, rdwr, len, TC_TIMEOUT_UINT_MS)) {
            return false;
        }
        waitAndDeactivateCS();
        return true;
    }
};

#endif

#endif
