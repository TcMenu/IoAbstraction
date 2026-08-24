
#ifdef BUILD_FOR_STM32CUBE_CMAKE

#include "StmCubeDigital.h"

#include "BasicIoAbstraction.h"
#include "IoLogging.h"

static StmGpioDesc ioaPins[STM32_IOA_GPIO_ARR_SIZE] = {};
static RawIntHandler interruptPinMappings[16]  = {};

void appendIoaPin(const StmGpioDesc& desc) {
    if (desc.getIoaPin() >= STM32_IOA_GPIO_ARR_SIZE) {
        serlogF2(SER_ERROR, "Pin>STM32_IOA_GPIO_ARR_SIZE", desc.getIoaPin());
        return;
    }
    ioaPins[desc.getIoaPin()] = desc;
}

void BasicIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    // not supported on stmcube, pre-configure pins and append them
}

void BasicIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
    const auto& gpioDef = ioaPins[pin];
    if (gpioDef.getPort() != nullptr) {
        HAL_GPIO_WritePin(gpioDef.getPort(), gpioDef.getGpioPin(), value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

uint8_t BasicIoAbstraction::readValue(pinid_t pin) {
    const auto& gpioDef = ioaPins[pin];
    if (gpioDef.getPort() != nullptr) {
        return HAL_GPIO_ReadPin(gpioDef.getPort(), gpioDef.getGpioPin());
    }
    return 0;
}

void BasicIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
    const auto& gpioDef = ioaPins[pin];
    if (gpioDef.getPort() != nullptr) {
        interruptPinMappings[gpioDef.getGpioPin()%16] = interruptHandler;

        // Determine IRQ number based on pin
        IRQn_Type irqn;
        uint16_t gpioPin = gpioDef.getGpioPin();
        if (gpioPin == GPIO_PIN_0) irqn = EXTI0_IRQn;
        else if (gpioPin == GPIO_PIN_1) irqn = EXTI1_IRQn;
        else if (gpioPin == GPIO_PIN_2) irqn = EXTI2_IRQn;
        else if (gpioPin == GPIO_PIN_3) irqn = EXTI3_IRQn;
        else if (gpioPin == GPIO_PIN_4) irqn = EXTI4_IRQn;
        else if (gpioPin <= GPIO_PIN_9) irqn = EXTI9_5_IRQn;
        else irqn = EXTI15_10_IRQn;
        HAL_NVIC_SetPriority(irqn, 5, 0);
        HAL_NVIC_EnableIRQ(irqn);
    }
}

void BasicIoAbstraction::writePort(pinid_t port, uint8_t portVal) {
    // unsupported
}

uint8_t BasicIoAbstraction::readPort(pinid_t port) {
    // unsupported
    return 0xff;
}

BasicIoAbstraction internalIoAbstraction;

void stmIntHasTriggered(uint16_t pin) {
    if (pin > 15) return;
    if (interruptPinMappings[pin]) {
        interruptPinMappings[pin]();
    }
}

const StmGpioDesc& getMappingAtPosition(uint16_t pin) {
    return ioaPins[pin % 16];
}

IoAbstractionRef internalDigitalIo() {
    return &internalIoAbstraction;
}

#endif
