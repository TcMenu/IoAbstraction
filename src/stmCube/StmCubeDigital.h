
#ifndef TESTLTDC_STMCUBEDIGITAL_H
#define TESTLTDC_STMCUBEDIGITAL_H
#include <cmath>

/**
 * @file StmCubeDigital.h
 * @brief This file contains the digital mappings for direct StmCube support.
 * This file contains StmCube mappings that make digital IO functions available within IoAbstraction. Allowing for
 * use of internal classes like RotaryEncoder and SwitchInput. The support works by allowing a configurable number
 * of GPIO to be mapped to IoAbstraction pin numbers. Interrupts can be attached to the GPIO pin, but only if the
 * pin was configured as such in StmCubeMX. Interrupts work exactly as the do on the platform, I.E. you implement
 * the weak handler method, and then call the `stmIntHasTriggered` method to tell us it has. This allows you to handle
 * interrupts too.
 */


#define INPUT 0x01
#define INPUT_PULLUP 0x02
#define OUTPUT 0xff
#define RISING 0x01
#define FALLING 0x02
#define CHANGE 0x03
#define PROGMEM
#define HIGH 1
#define LOW 0

#define bitRead(value, bit) (((value) & (1 << (bit))) != 0)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#include <PlatformDetermination.h>


/**
 * This class represents a GPIO pin on the STM32Cube platform. You can use it to configure the device digital IO so
 * that it knows that for example [GPIOA, Pin3] is mapped in IoAbstraction to Pin 3. This is because IoAbstraction uses
 * pin numbers to identify the GPIO pins, so it needs a way to map the STM provided Port/pins to the pin numbers.
 */
class StmGpioDesc {
    GPIO_TypeDef *port;
    uint16_t pinNum;
    uint16_t ioaPin;
public:
    StmGpioDesc() = default;
    StmGpioDesc(GPIO_TypeDef *port, uint16_t pinNum, uint16_t ioaPin) : port(port), pinNum(pinNum), ioaPin(ioaPin) {}
    StmGpioDesc(const StmGpioDesc &other) = default;
    StmGpioDesc(StmGpioDesc &&other) = default;
    StmGpioDesc& operator=(const StmGpioDesc &other) = default;

    [[nodiscard]] uint16_t getIoaPin() const { return pinNum; }
    [[nodiscard]] uint16_t getGpioPin() const { return pinNum; }
    [[nodiscard]] GPIO_TypeDef* getPort() const { return port; }

    [[nodiscard]] uint16_t getKey() const { return ioaPin; }
};

/**
 * STMCube uses both a port and Pin to identify a GPIO. However, IoAbstraction uses pin numbers to identify the GPIO pins,
 * so it needs a way to map the STM provided Port/pins to the pin numbers. The provided mapping will use the IOA pin
 * as the key to identify the GPIO pin.
 *
 * IoAbstraction's GPIO is used heavily inside the library for reading pins values and writing new ones. In order to
 * make this class interrupt safe on all processors we have avoided using a list to store the pins and instead use an
 * array of pin number to GPIO definition. This gives you 16 GPIO that you can use with digital or analog and makes it
 * very fast indeed. You can increase this number by setting the STM32_IOA_GPIO_ARR_SIZE (default 16) to a larger number.
 *
 * It is assumed that the GPIO has already been configured for the desired mode and speed before calling this method.
 * @param desc the pin mapping for IoAbstraction device to use must be smaller than STM32_IOA_GPIO_ARR_SIZE
 */
void appendIoaPin(const StmGpioDesc& desc);

/**
 * Unlike other platforms, STMCube uses a single interrupt handler usually provided in main using a weak method that
 * we can override. When you override this method and you're using attachInterrupt then you should call this function
 * to ensure that the interrupt is handled correctly. It is interrupt safe.
 * @param pin the GPIO pin that came in the interrupt handler.
 */
void stmIntHasTriggered(uint16_t pin);

/**
 * Get hold of the GPIO mapping structure for the given pin.
 * @param pin the pin number to get the mapping for.
 * @return the mapping for the given pin.
 */
const StmGpioDesc& getMappingAtPosition(uint16_t pin);

// If you need more GPIO pins than the default 32 you can increase this number from 16 upwards in the build flags.
#ifndef STM32_IOA_GPIO_ARR_SIZE
#define STM32_IOA_GPIO_ARR_SIZE 16
#endif

#endif //TESTLTDC_STMCUBEDIGITAL_H
