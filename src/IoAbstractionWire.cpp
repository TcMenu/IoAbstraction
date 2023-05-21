/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include <IoAbstractionWire.h>
#include "wireHelpers.h"

PCF8574IoAbstraction::PCF8574IoAbstraction(uint8_t addr, uint8_t interruptPin, WireType wireImplementation, bool mode16Bit, bool invertedLogic) : lastRead{}, toWrite{} {
	this->wireImpl = (wireImplementation != nullptr) ? wireImplementation : defaultWireTypePtr;
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
    this->wireImpl = (wireImpl != nullptr) ? wireImpl : defaultWireTypePtr;
    this->address = address;
	this->intPinA = intPinA;
	this->intPinB = intPinB;
	this->intMode = intMode;
}

MCP23017IoAbstraction::MCP23017IoAbstraction(uint8_t address, Mcp23xInterruptMode intMode, pinid_t intPinA, WireType wireImpl) :
        Standard16BitDevice() {
    this->wireImpl = (wireImpl != nullptr) ? wireImpl : defaultWireTypePtr;
    this->address = address;
    this->intPinA = intPinA;
    this->intPinB = IO_PIN_NOT_DEFINED;
    this->intMode = intMode;
}

MCP23017IoAbstraction::MCP23017IoAbstraction(uint8_t address, WireType wireImpl) :
        Standard16BitDevice() {
    this->wireImpl = (wireImpl != nullptr) ? wireImpl : defaultWireTypePtr;
    this->address = address;
    this->intPinA = this->intPinB = IO_PIN_NOT_DEFINED;
    this->intMode = NOT_ENABLED;
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

void MCP23017IoAbstraction::resetDevice(int resetPin) {
    internalDigitalDevice().pinMode(resetPin, OUTPUT);
    internalDigitalDevice().digitalWriteS(resetPin, LOW);
    taskManager.yieldForMicros(100);
    internalDigitalDevice().digitalWriteS(resetPin, HIGH);
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
    else if(pin < 12) return AW9523_LED_DIM_START + (pin - 8);
    else return AW9523_LED_DIM_START + pin;
}

AW9523IoAbstraction::AW9523IoAbstraction(uint8_t addr, pinid_t intPin, WireType wirePtr) : Standard16BitDevice(),
        i2cAddress(addr), interruptPin(intPin) {
    wireImpl = (wirePtr != nullptr) ? wirePtr : defaultWireTypePtr;
}

void AW9523IoAbstraction::initDevice() {
    markInitialised();

    // reset the device
    softwareReset();

    // turn off all interrupts
    wireWriteReg16(wireImpl, i2cAddress, AW9523_INTERRUPT_CTRL_16, 0xFFFF);

    // set everything to output, low
    wireWriteReg16(wireImpl, i2cAddress, AW9523_PORT_DIRECTION_16, 0x0);

    // full current, push/pull port 0.
    writeGlobalControl(true);
}

void AW9523IoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler intHandler, uint8_t mode) {
     if(isInitNeeded()) initDevice();

    // only if there's an interrupt pin set
    if(interruptPin == IO_PIN_NOT_DEFINED) {
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
    if(isInitNeeded()) initDevice();

    if(mode == INPUT || mode == INPUT_PULLUP) {
        setReadPort(pin < 8 ? 0 : 1);
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_PORT_DIRECTION_16, pin, true);
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_LED_MODE_16, pin, true);
    } else if(mode == OUTPUT) {
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_PORT_DIRECTION_16, pin, false);
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_LED_MODE_16, pin, true);
    } else if(mode == AW9523_LED_OUTPUT) {
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_PORT_DIRECTION_16, pin, false);
        toggleBitInRegister16(wireImpl, i2cAddress, AW9523_LED_MODE_16, pin, false);
    } else {
        serlogF3(SER_ERROR, "AW9523 mode error ", pin, mode);
        return;
    }
}

bool AW9523IoAbstraction::runLoop() {
    if(isInitNeeded()) initDevice();

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
    if(isInitNeeded()) initDevice();
    wireWriteReg8(wireImpl, i2cAddress, getAw9523LedDimRegister(pin), pwr);
    if(bitRead(toWrite, pin) == 0 && pwr != 0) {
        digitalWriteS(pin, true);
    }
}

void AW9523IoAbstraction::softwareReset() {
    wireWriteReg8(wireImpl, i2cAddress, AW9523_SW_RESET_REG, 0);
}

uint8_t AW9523IoAbstraction::deviceId() {
    return wireReadReg8(wireImpl, i2cAddress, AW9523_CHIP_IDENTIFIER);
}

void AW9523IoAbstraction::writeGlobalControl(bool pushPullP0, AW9523CurrentControl maxCurrentMode) {
    uint8_t params = maxCurrentMode & 0x03;
    bitWrite(params, 4, pushPullP0);
    wireWriteReg8(wireImpl, i2cAddress, AW9523_GLOBAL_CONTROL, params);
}

void AW9523AnalogAbstraction::initPin(pinid_t pin, AnalogDirection direction) {
    if(direction == DIR_PWM || direction == DIR_OUT) {
        theAbstraction.pinMode(pin, AW9523_LED_OUTPUT);
    } else {
        serlogF2(SER_ERROR, "AW9523 No AnalogIn", pin);
    }
}

MPR121IoAbstraction::MPR121IoAbstraction(uint8_t addr, pinid_t intPin, WireType wirePtr) : Standard16BitDevice(),
                                                                                           i2cAddress(addr), interruptPin(intPin) {
    wireImpl = (wirePtr != nullptr) ? wirePtr : defaultWireTypePtr;
}

void MPR121IoAbstraction::softwareReset() {
    // perform a reset and stop the chip.
    wireWriteReg8(wireImpl, i2cAddress, MPR121_SOFT_RESET, MPR121_SOFT_RESET_VALUE);
    wireWriteReg8(wireImpl, i2cAddress, MPR121_ELECTRODE_CONFIG, 0);
}

void MPR121IoAbstraction::setPinLedCurrent(pinid_t pin, uint8_t pwr) {
    if(pin < 4 || pin > 11 || pwr > 15) {
        serlogF3(SER_ERROR, "LED err", pin, pwr);
        return;
    }
    pin -= 4;
    auto reg = MPR121_LED_PWM_0 + (pin / 2);
    write4BitToReg8(wireImpl, i2cAddress, reg, pin % 2 == 0, pwr);
}

void MPR121IoAbstraction::writeReg8(uint8_t reg, uint8_t data) {
    wireWriteReg8(wireImpl, i2cAddress, reg, data);
}

void MPR121IoAbstraction::writeReg16(uint8_t reg, uint16_t data) {
    wireWriteReg16(wireImpl, i2cAddress, reg, data);
}

uint8_t MPR121IoAbstraction::readReg8(uint8_t reg) {
    return wireReadReg8(wireImpl, i2cAddress, reg);
}

uint16_t MPR121IoAbstraction::readReg16(uint8_t reg) {
    return wireReadReg16(wireImpl, i2cAddress, reg);
}

void MPR121IoAbstraction::configureDebounce(uint8_t debounceTouch, uint8_t debounceRelease) {
    uint8_t data = (debounceRelease << 4) | (debounceTouch & 0x7);
    wireWriteReg8(wireImpl, i2cAddress, MPR121_DEBOUNCE_REG, data);
}

void MPR121IoAbstraction::electrodeSettingsForPin(pinid_t pin, uint8_t touchThreshold, uint8_t releaseThreshold, uint8_t current, uint8_t chargeTime) {
    wireWriteReg8(wireImpl, i2cAddress, uint8_t(MPR121_ELECTRODE_CURRENT_0 + pin), current);
    auto reg = MPR121_CHARGE_TIME_0 + (pin / 2);
    write4BitToReg8(wireImpl, i2cAddress, reg, pin % 1 == 0, chargeTime);

    reg = MPR121_TCH_REL_THRESHOLD + (pin * 2);
    wireWriteReg8(wireImpl, i2cAddress, reg, touchThreshold);
    wireWriteReg8(wireImpl, i2cAddress, reg + 1, releaseThreshold);
}

uint16_t MPR121IoAbstraction::read2ndFilteredData(uint8_t pin) {
    return wireReadReg16(wireImpl, i2cAddress, MPR121_ELECTRODE_DATA_2ND + (pin * 2));
}

uint16_t MPR121IoAbstraction::getOutOfRangeRegister() {
    return wireReadReg16(wireImpl, i2cAddress, MPR121_OOR_STATUS_16);
}

void MPR121IoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    if(mode == LED_CURRENT_OUTPUT || mode == OUTPUT) {
        if(pin < 4) return;
        int gpioPinNo = pin - 4;
        toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_ENABLE, gpioPinNo, true);
        toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_DIRECTION_0, gpioPinNo, true);
        toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_CONTROL_0, gpioPinNo, mode == LED_CURRENT_OUTPUT);
        toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_CONTROL_1, gpioPinNo, mode == LED_CURRENT_OUTPUT);
    } else if(mode == INPUT || mode == INPUT_PULLUP){
        // if the touch support has already been prepared for our pin, there is nothing to do here. It is assumed that
        // in this case you have already configured the touch parameters. 
        if(maximumTouchPin < pin  && pin > 4) {
            int gpioPinNo = pin - 4;
            toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_ENABLE, gpioPinNo, true);
            toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_DIRECTION_0, gpioPinNo, false);
            toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_CONTROL_0, gpioPinNo, mode == INPUT_PULLUP);
            toggleBitInRegister8(wireImpl, i2cAddress, MPR121_GPIO_CONTROL_1, gpioPinNo, mode == INPUT_PULLUP);
        }
    }
}

void MPR121IoAbstraction::begin(int maxTouchPin, MPR121ConfigType configType, uint8_t configReg1, uint8_t configReg2) {
    // go into stop mode before init.
    writeReg8(MPR121_ELECTRODE_CONFIG, 0);

    // set up the basic touch configuration
    writeReg8(MPR121_MHD_RISING, 0x01);
    writeReg8(MPR121_NHD_RISING, 0x01);
    writeReg8(MPR121_NCL_RISING, 0x0E);
    writeReg8(MPR121_FDL_RISING, 0x00);

    writeReg8(MPR121_FDL_FALLING, 0x01);
    writeReg8(MPR121_FDL_FALLING, 0x05);
    writeReg8(MPR121_FDL_FALLING, 0x01);
    writeReg8(MPR121_FDL_FALLING, 0x00);

    writeReg8(MPR121_FDL_TOUCHED, 0x00);
    writeReg8(MPR121_FDL_TOUCHED, 0x00);
    writeReg8(MPR121_FDL_TOUCHED, 0x00);

    // write the general configuration registers.
    writeReg8(MPR121_AFE_CONFIG_1, configReg1);
    writeReg8(MPR121_AFE_CONFIG_2, configReg2);

    if(configType == MPR121_AUTO_CONFIG) {
        // These values are as per Adafruit library https://github.com/adafruit/Adafruit_MPR121/blob/master/Adafruit_MPR121.h
        // as most users will be using that board or a clone thereof.
        writeReg8(MPR121_AUTO_CONFIG_0, 0x0B);

        // correct values for Vdd = 3.3V
        writeReg8(MPR121_UPPER_LIMIT, 200);  // ((Vdd - 0.7)/Vdd) * 256
        writeReg8(MPR121_TARGET_LIMIT, 180); // UPLIMIT * 0.9
        writeReg8(MPR121_LOWER_LIMIT, 130);  // UPLIMIT * 0.65
    }

    uint8_t ecrSetting = 0b10000000 | (maxTouchPin + 1);
    writeReg8(MPR121_ELECTRODE_CONFIG, ecrSetting);
    maximumTouchPin = maxTouchPin;
}

bool MPR121IoAbstraction::runLoop() {
    // is GPIO or touch on GPIO pins
    if(maximumTouchPin != 0 && !isReadPortSet(0)) {
        lastRead = readReg16(MPR121_TOUCH_STATUS_16);
    } else if(isReadPortSet(0)) {
        lastRead = readReg16(MPR121_TOUCH_STATUS_16);

        // GPIO input mode is only supported with up to 4 touch sensors.
        if(maximumTouchPin < 4) {
            auto gpioRead = readReg8(MPR121_GPIO_DATA);
            lastRead = lastRead | (gpioRead << 4);
        }
    }
    
    if(isWritePortSet(0) || isWritePortSet(1)) {
        writeReg8(MPR121_GPIO_DATA, (toWrite >> 4));
    }
    clearChangeFlags();

    return true;
}

void MPR121IoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler intHandler, uint8_t mode) {
    // if there's an interrupt pin set
    if(interruptPin == 0xff) return;

    internalDigitalDevice().pinMode(interruptPin, INPUT_PULLUP);
    internalDigitalDevice().attachInterrupt(interruptPin, intHandler, FALLING);
}

void MPR121AnalogAbstraction::initPin(pinid_t pin, AnalogDirection direction) {
    if(direction == DIR_PWM || direction == DIR_OUT) {
        theAbstraction.pinMode(pin, AW9523_LED_OUTPUT);
    }
    // for input it is assumed that the user has already enabled touch
}
