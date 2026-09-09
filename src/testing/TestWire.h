
#ifndef IOA_TEST_WIRE_H
#define IOA_TEST_WIRE_H
#include <vector>

struct DataPacket {
    char data[32] = {};
    size_t len;
    uint8_t address;

    DataPacket(const char* data, size_t len, uint8_t address) : len(len), address(address) {
        strncpy(this->data, data, len);
    }
};

class MockI2cWrapper {
private:
    std::vector<DataPacket> receivedData;
    std::vector<DataPacket> dataToSend;
public:
    void init() {
        receivedData.clear();
        dataToSend.clear();
    }

    bool wireRead(uint8_t addr, uint8_t *dst, size_t len) {
        if (!dataToSend.empty()) {
            auto data = dataToSend.back();
            dataToSend.pop_back();
            if (data.len > len) return false;
            memcpy(dst, dataToSend[0].data, len);
        } else {
            return false;
        }
        return true;
    }

    bool wireWrite(uint8_t addr, const uint8_t *dst, size_t len, int retries, bool sendStop) {
        receivedData.emplace_back(reinterpret_cast<const char*>(dst), len, addr);
        return true;
    }
};

class SPIWithSettings {
public:
    bool transferSPI(uint8_t* rdwr, const size_t len) {
        return false;
    }

    bool write(const uint8_t* data, const size_t size) {
        return false;
    }
};
typedef MockI2cWrapper* WireType;

#endif
