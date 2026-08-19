
#ifndef TESTLTDC_MBEDSPI_H
#define TESTLTDC_MBEDSPI_H


#define TC_SPI_WRITE_AVAILABLE

class SPIWithSettings {
private:
    SPI* spiBus;
    uint32_t speed;
    pinid_t csPin = 0;
    bool initializedYet = false;
public:
    SPIWithSettings(SPI* bus, pinid_t cs) : spiBus(bus), speed(10000000), csPin(cs) {}
    SPIWithSettings(SPI* bus, pinid_t cs, uint32_t speed) : spiBus(bus), speed(speed), csPin(cs) {}
    SPIWithSettings(const SPIWithSettings&) = default;
    SPIWithSettings& operator=(const SPIWithSettings&)=default;

    void init() {
        internalDigitalDevice().pinMode(csPin, OUTPUT);
        internalDigitalDevice().digitalWrite(csPin, HIGH);
        initializedYet=true;
    }

    void waitABit() {
        asm volatile("nop \n nop \n nop");
    }

    void waitAndActiveCS() {
        if(!initializedYet) {
            init();
        }

        internalDigitalDevice().digitalWrite(csPin, LOW);
        waitABit();
    }

    void waitAndDeactivateCS() {
        waitABit();
        internalDigitalDevice().digitalWrite(csPin, HIGH);
        waitABit();
    }

    bool write(const uint8_t* data, size_t size) {
        waitAndActiveCS();
        char sz[1];
        int written = spiBus->write((const char*)data, size, sz, 0);
        waitAndDeactivateCS();
        return written == size;
    }

    bool transferSPI(uint8_t* rdwr, size_t len) {
        waitAndActiveCS();
        int written = spiBus->write((const char*)rdwr, len, (char*)rdwr, len);
        waitAndDeactivateCS();
        return written == len;
    }
};


#endif //TESTLTDC_MBEDSPI_H
