/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <IoAbstractionWire.h>

PCF8574IoAbstraction::PCF8574IoAbstraction(uint8_t addr, uint8_t interruptPin, WireType wireImplementation, bool mode16Bit, bool invertedLogic) : lastRead{}, toWrite{} {
	this->wireImpl = wireImplementation;
	this->address = addr;
	this->interruptPin = interruptPin;
    flags = 0;
    bitWrite(flags, NEEDS_WRITE_FLAG, true);
    bitWrite(flags, PCF8575_16BIT_FLAG, mode16Bit);
    bitWrite(flags, INVERTED_LOGIC, invertedLogic);
}

void PCF8574IoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
	bool invertedLogic = bitRead(flags, INVERTED_LOGIC);

	/*
	 When inverted logic is set to true, we do the inversion in the runLoop().
	 However, inputs need to be always set to HIGH in order the read to work.
	 So it is necessary to flip the value here.
	 */
	if (mode == INPUT || mode == INPUT_PULLUP) {
		overrideReadFlag();
		writeValue(pin, !invertedLogic ? HIGH : LOW);
	}
	else {
		writeValue(pin, LOW);
	}
    bitWrite(flags, NEEDS_WRITE_FLAG, true);
}

void PCF8574IoAbstraction::writeValue(pinid_t pin, uint8_t value) {
    int port = (pin > 7) ? 1 : 0;
	bitWrite(toWrite[port], (pin % 8), value);
    bitWrite(flags, NEEDS_WRITE_FLAG, true);
}

uint8_t PCF8574IoAbstraction::readValue(pinid_t pin) {
    int port = (pin > 7) ? 1 : 0;
    return (lastRead[port] & (1 << (pin % 8))) ? HIGH : LOW;
}

uint8_t PCF8574IoAbstraction::readPort(pinid_t pin) {
    int port = (pin > 7) ? 1 : 0;
    return lastRead[port];
}

void PCF8574IoAbstraction::writePort(pinid_t pin, uint8_t value) {
    int port = (pin > 7) ? 1 : 0;
    toWrite[port] = value;
    bitWrite(flags, NEEDS_WRITE_FLAG, true);
}

bool PCF8574IoAbstraction::runLoop(){
    bool writeOk = true;
    size_t bytesToTransfer = bitRead(flags, PCF8575_16BIT_FLAG) ? 2 : 1;
    bool invertedLogic = bitRead(flags, INVERTED_LOGIC);

    if (bitRead(flags, NEEDS_WRITE_FLAG)) {
        bitWrite(flags, NEEDS_WRITE_FLAG, false);

        uint8_t dataToWrite[2];
        dataToWrite[0] = invertedLogic ? ~toWrite[0] : toWrite[0];
        dataToWrite[1] = invertedLogic ? ~toWrite[1] : toWrite[1];

        writeOk = ioaWireWriteWithRetry(wireImpl, address, dataToWrite, bytesToTransfer);
    }

    if(bitRead(flags, PINS_CONFIGURED_READ_FLAG)) {
        writeOk = writeOk && ioaWireRead(wireImpl, address, lastRead, bytesToTransfer);

        if (invertedLogic) {
            lastRead[0] = ~lastRead[0];
            lastRead[1] = ~lastRead[1];
        }
    }
    return writeOk;
}


void PCF8574IoAbstraction::attachInterrupt(pinid_t /*pin*/, RawIntHandler intHandler, uint8_t /*mode*/) {
	// if there's an interrupt pin set
	if(interruptPin == 0xff) return;

    auto deviceIo = internalDigitalDevice();
    deviceIo.pinDirection(interruptPin, INPUT_PULLUP);
    deviceIo.attachInterrupt(interruptPin, intHandler, FALLING);
}

BasicIoAbstraction* ioFrom8574(uint8_t addr, pinid_t interruptPin, WireType wireImpl, bool invertedLogic) {
	return new PCF8574IoAbstraction(addr, interruptPin, wireImpl, invertedLogic);
}

BasicIoAbstraction* ioFrom8575(uint8_t addr, pinid_t interruptPin, WireType wireImpl, bool invertedLogic) {
    return new PCF8574IoAbstraction(addr, interruptPin, wireImpl, true, invertedLogic);
}

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
    ioaWireWriteWithRetry(wireType, addr, &reg, 1, 0);

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

void toggleBitInRegister16(WireType wireType, uint8_t addr, uint8_t regAddr, uint8_t theBit, bool value) {
    uint16_t reg = wireReadReg16(wireType, addr, regAddr);
    bitWrite(reg, theBit, value);

    // for debugging to see the commands being sent, uncomment below
    serlogF4(SER_IOA_DEBUG, "toggle(regAddr, bit, toggle): ", regAddr, theBit, value);
    serlogFHex(SER_IOA_DEBUG, "Value: ", reg);
    // end debugging code

    wireWriteReg16(wireType, addr, regAddr, reg);
}

//
// MCP23017 definitions
//

// definitions of register addresses.

#define IODIR_ADDR       0x00
#define IPOL_ADDR        0x02
#define GPINTENA_ADDR    0x04
#define DEFVAL_ADDR      0x06
#define INTCON_ADDR      0x08
#define IOCON_ADDR       0x0a
#define GPPU_ADDR        0x0c
#define INTF_ADDR        0x0e
#define INTCAP_ADDR      0x10
#define GPIO_ADDR        0x12
#define OUTLAT_ADDR      0x14

// definitions for the IO control register

#define IOCON_HAEN_BIT  3
#define IOCON_SEQOP_BIT  5
#define IOCON_MIRROR_BIT  6
#define IOCON_BANK_BIT  7

MCP23017IoAbstraction::MCP23017IoAbstraction(uint8_t address, Mcp23xInterruptMode intMode, pinid_t intPinA, pinid_t intPinB, WireType wireImpl) :
        Standard16BitDevice() {
	this->wireImpl = wireImpl;
	this->address = address;
	this->intPinA = intPinA;
	this->intPinB = intPinB;
	this->intMode = intMode;
}

void MCP23017IoAbstraction::initDevice() {
	uint8_t controlReg = (wireReadReg16(wireImpl, address, IOCON_ADDR) & 0xff);
	
	if(intPinB == 0xff && intPinA != 0xff) {
		bitSet(controlReg, IOCON_MIRROR_BIT);
	}
	else if(intPinA != 0xff) {
		bitClear(controlReg, IOCON_MIRROR_BIT);
	}

	bitClear(controlReg, IOCON_BANK_BIT);
	bitClear(controlReg, IOCON_SEQOP_BIT);

	uint16_t regToWrite = controlReg | (((uint16_t)controlReg) << 8U);
    wireWriteReg16(wireImpl, address, IOCON_ADDR, regToWrite);

    markInitialised();
}

void MCP23017IoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
	if(isInitNeeded()) initDevice();

	toggleBitInRegister16(wireImpl, address, IODIR_ADDR, pin, (mode == INPUT || mode == INPUT_PULLUP));
	toggleBitInRegister16(wireImpl, address, GPPU_ADDR, pin, mode == INPUT_PULLUP);

    setReadPort((pin < 8) ? 0 : 1);
}

bool MCP23017IoAbstraction::runLoop() {
	if(isInitNeeded()) initDevice();

	bool writeOk = true;

	bool flagA = isWritePortSet(0);
	bool flagB = isWritePortSet(1);
	if(flagA && flagB) // write on both ports
		writeOk = wireWriteReg16(wireImpl, address, OUTLAT_ADDR, toWrite);
	else if(flagA) 
		writeOk = wireWriteReg8(wireImpl, address, OUTLAT_ADDR, toWrite);
	else if(flagB)
		writeOk = wireWriteReg8(wireImpl, address, OUTLAT_ADDR + 1, toWrite >> 8);

	clearChangeFlags();

	flagA = isReadPortSet(0);
	flagB = isReadPortSet(1);
	if(flagA && flagB)
		lastRead = wireReadReg16(wireImpl, address, GPIO_ADDR);
	else if(flagA)
		lastRead = wireReadReg8(wireImpl, address, GPIO_ADDR);
	else if(flagB)
		lastRead = wireReadReg8(wireImpl, address, GPIO_ADDR + 1) << 8U;

	return writeOk;
}

void MCP23017IoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler intHandler, uint8_t mode) {
	// only if there's an interrupt pin set
	if(intPinA == 0xff) return;

	auto  inbuiltIo = internalDigitalDevice();
	uint8_t pm = (intMode == ACTIVE_HIGH_OPEN || intMode == ACTIVE_LOW_OPEN) ? INPUT_PULLUP : INPUT;
	uint8_t im = (intMode == ACTIVE_HIGH || intMode == ACTIVE_HIGH_OPEN) ? RISING : FALLING;
	if(intPinB == 0xff) {
        inbuiltIo.pinMode(intPinA, pm);
        inbuiltIo.attachInterrupt(intPinA, intHandler, im);
	}
	else {
        inbuiltIo.pinMode(intPinA, pm);
        inbuiltIo.attachInterrupt(intPinA, intHandler, im);
        inbuiltIo.pinMode(intPinB, pm);
        inbuiltIo.attachInterrupt(intPinB, intHandler, im);
    }

	toggleBitInRegister16(wireImpl, address, GPINTENA_ADDR, pin, true);
	toggleBitInRegister16(wireImpl, address, INTCON_ADDR, pin, mode != CHANGE);
	toggleBitInRegister16(wireImpl, address, DEFVAL_ADDR, pin, mode == FALLING);
}

void MCP23017IoAbstraction::setInvertInputPin(pinid_t pin, bool shouldInvert) {
    toggleBitInRegister16(wireImpl, address, IPOL_ADDR, pin, shouldInvert);
}

IoAbstractionRef ioFrom23017(pinid_t addr, WireType wireImpl) {
	return ioFrom23017IntPerPort(addr, NOT_ENABLED, 0xff, 0xff, wireImpl);
}

IoAbstractionRef ioFrom23017(uint8_t addr, Mcp23xInterruptMode intMode, pinid_t interruptPin, WireType wireImpl) {
	return ioFrom23017IntPerPort(addr, intMode, interruptPin, 0xff, wireImpl);
}

IoAbstractionRef ioFrom23017IntPerPort(pinid_t addr, Mcp23xInterruptMode intMode, pinid_t intPinA, pinid_t intPinB, WireType wireImpl) {
	return new MCP23017IoAbstraction(addr, intMode, intPinA, intPinB, wireImpl);
}

//
// Standard16BitDevice
//

#define STD16_CHANGE_PORTA_BIT 0
#define STD16_CHANGE_PORTB_BIT 1
#define STD16_READER_PORTA_BIT 2
#define STD16_READER_PORTB_BIT 3
#define STD16_NEEDS_INIT 4

//
// AW9523IoAbstraction implementation
//

Standard16BitDevice::Standard16BitDevice() {
    lastRead = toWrite = 0;
    flags = 0;
    bitSet(flags, STD16_NEEDS_INIT);
}

uint8_t Standard16BitDevice::readValue(pinid_t pin) {
    return bitRead(lastRead, pin);
}

void Standard16BitDevice::writeValue(pinid_t pin, uint8_t value) {
    if(bitRead(flags, STD16_NEEDS_INIT)) initDevice();

    bitWrite(toWrite, pin, value);
    bitSet(flags, (pin < 8) ? STD16_CHANGE_PORTA_BIT : STD16_CHANGE_PORTB_BIT);

}

uint8_t Standard16BitDevice::readPort(pinid_t pin) {
    return (pin < 8) ? (lastRead & 0xff) : (lastRead >> 8);
}

void Standard16BitDevice::writePort(pinid_t pin, uint8_t value) {
    if(pin < 8) {
        toWrite &= 0xff00;
        toWrite |= value;
        bitSet(flags, STD16_CHANGE_PORTA_BIT);
    }
    else {
        toWrite &= 0x00ff;
        toWrite |= ((uint16_t)value << 8);
        bitSet(flags, STD16_CHANGE_PORTB_BIT);
    }
}

void Standard16BitDevice::clearChangeFlags() {
    bitClear(flags, STD16_CHANGE_PORTA_BIT);
    bitClear(flags, STD16_CHANGE_PORTB_BIT);
}

bool Standard16BitDevice::isReadPortSet(int port) const {
    auto bitNum = (port == 0) ? STD16_READER_PORTA_BIT : STD16_READER_PORTB_BIT;
    return bitRead(flags, bitNum);
}

void Standard16BitDevice::setReadPort(int port) {
    auto bitNum = (port == 0) ? STD16_READER_PORTA_BIT : STD16_READER_PORTB_BIT;
    bitSet(flags, bitNum);
}

bool Standard16BitDevice::isWritePortSet(int port) const {
    auto bitNum = (port == 0) ? STD16_CHANGE_PORTA_BIT : STD16_CHANGE_PORTB_BIT;
    return bitRead(flags, bitNum);
}

bool Standard16BitDevice::isInitNeeded() const {
    return bitRead(flags, STD16_NEEDS_INIT);
}

void Standard16BitDevice::markInitialised() {
    bitClear(flags, STD16_NEEDS_INIT);
}

#define AW9523_SW_RESET_REG 0x7F
#define AW9523_INPUT_READ_16 0x00
#define AW9523_OUTPUT_WRITE_16 0x02
#define AW9523_PORT_DIRECTION_16 0x04
#define AW9523_INTERRUPT_CTRL_16 0x06
#define AW9523_CHIP_IDENTIFIER 0x10
#define AW9523_GLOBAL_CONTROL 0x11
#define AW9523_LED_MODE_16 0x12
#define AW9523_LED_DIM_START 0x20

inline uint8_t getAw9523LedDimRegister(pinid_t pin) {
    if(pin < 8) return AW9523_LED_DIM_START + 4 + pin;
    else if(pin < 12) return AW9523_LED_DIM_START + pin;
    else return AW9523_LED_DIM_START + pin;
}

AW9523IoAbstraction::AW9523IoAbstraction(uint8_t addr, pinid_t intPin, WireType wireImpl) : Standard16BitDevice(), wireImpl(wireImpl),
        i2cAddress(addr), interruptPin(intPin) {}

void AW9523IoAbstraction::initDevice() {
    // turn off all interrupts by default
    wireWriteReg16(wireImpl, i2cAddress, AW9523_INTERRUPT_CTRL_16, 0xFFFF);
    wireWriteReg16(wireImpl, i2cAddress, AW9523_PORT_DIRECTION_16, 0x0);
    writeGlobalControl(true);

    markInitialised();
}

void AW9523IoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler intHandler, uint8_t mode) {
    // only if there's an interrupt pin set
    if(interruptPin == 0xff) {
        serlogF(SER_ERROR, "AW9523 no int pin");
        return;
    }

    // enable the interrupt on the device itself.
    internalDigitalDevice().pinMode(interruptPin, INPUT_PULLUP);
    internalDigitalDevice().attachInterrupt(interruptPin, intHandler, CHANGE);

    // now enable interrupt support for this pin
    toggleBitInRegister16(wireImpl, i2cAddress, AW9523_INTERRUPT_CTRL_16, pin, false);
}

void AW9523IoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    if(mode == INPUT) {
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_PORT_DIRECTION_16, pin, false);
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_LED_MODE_16, pin, true);
    } else if(mode == OUTPUT) {
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_PORT_DIRECTION_16, pin, true);
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_LED_MODE_16, pin, true);
    } else {
        serlogF3(SER_ERROR, "AW9523 mode error ", pin, mode);
        return;
    }
}

bool AW9523IoAbstraction::runLoop() {
    bool writeOk = true;
    bool port0 = isWritePortSet(0);
    bool port1 = isWritePortSet(1);
    if(port0 && port1) // write on both ports
        writeOk = wireWriteReg16(wireImpl, i2cAddress, AW9523_OUTPUT_WRITE_16, toWrite);
    else if(port0)
        writeOk = wireWriteReg8(wireImpl, i2cAddress, AW9523_OUTPUT_WRITE_16, toWrite);
    else if(port1)
        writeOk = wireWriteReg8(wireImpl, i2cAddress, AW9523_OUTPUT_WRITE_16 + 1, toWrite >> 8U);

    clearChangeFlags();

    port0 = isReadPortSet(0);
    port1 = isReadPortSet(1);
    if(port0 && port1) {
        lastRead = wireReadReg16(wireImpl, i2cAddress, AW9523_INPUT_READ_16);
    } else if(port0){
        lastRead = wireReadReg8(wireImpl, i2cAddress, AW9523_INPUT_READ_16);
    } else if(port1) {
        lastRead = wireReadReg8(wireImpl, i2cAddress, AW9523_INPUT_READ_16) << 8U;
    }
    return writeOk;
}

void AW9523IoAbstraction::setPinLedCurrent(pinid_t pin, uint8_t pwr) {
    toggleBitInRegister16(wireImpl, i2cAddress, AW9523_LED_MODE_16, pin, false);
    wireWriteReg8(wireImpl, i2cAddress, getAw9523LedDimRegister(pin), pwr);
}

void AW9523IoAbstraction::softwareReset() {
    wireWriteReg8(wireImpl, i2cAddress, AW9523_SW_RESET_REG, 0xff);
}

void AW9523IoAbstraction::writeGlobalControl(bool pushPullP0, uint8_t maxCurrentMode) {
    uint8_t params = maxCurrentMode & 0x04;
    auto openDrain = !pushPullP0;
    bitWrite(params, 4, openDrain);
    wireWriteReg8(wireImpl, i2cAddress, AW9523_GLOBAL_CONTROL, params);
}
