
#include "PicoDigitalIO.h"
#include "../IoAbstraction.h"
#include <SimpleCollections.h>

#ifdef BUILD_FOR_PICO_CMAKE

void BasicIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    gpio_init(pin);
    if(mode == INPUT || mode == INPUT_PULLUP) {
        gpio_set_dir(pin, GPIO_IN);
        if(mode == INPUT_PULLUP) gpio_pull_up(pin);
    } else {
        gpio_set_dir(pin, GPIO_OUT);
    }
}

void BasicIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
    gpio_put(pin, value != 0);
}

uint8_t BasicIoAbstraction::readValue(pinid_t pin) {
    return gpio_get(pin);
}

class GpioIntHandler {
private:
    RawIntHandler underlyingHandler;
    pinid_t pinNum;
public:
    GpioIntHandler() : underlyingHandler(nullptr), pinNum(0xff) {}
    void initWith(RawIntHandler h, pinid_t p) {
        underlyingHandler = h;
        pinNum = p;
    }
    void triggered() {if(underlyingHandler) underlyingHandler();}
    [[nodiscard]] pinid_t getPin() const { return pinNum; }
};

#define MAX_INT_HANDLER 8
GpioIntHandler handlers[MAX_INT_HANDLER];

void findAndRunHanderFn(uint gpio, uint32_t event_mask) {
    for(int i=0; i<MAX_INT_HANDLER; i++) {
        if(gpio == handlers[i].getPin()) {
            handlers[i].triggered();
        }
    }
}

void BasicIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
    for(int i=0; i<MAX_INT_HANDLER; i++) {
        if(handlers[i].getPin() == 0xff) {
            handlers[i].initWith(interruptHandler, pin);
            uint32_t eventMask = 0;
            if(mode == CHANGE || mode == RISING) eventMask |= GPIO_IRQ_EDGE_RISE;
            if(mode == CHANGE || mode == FALLING) eventMask |= GPIO_IRQ_EDGE_FALL;
            gpio_set_irq_enabled_with_callback(pin, eventMask, true, findAndRunHanderFn);
            return;
        }
    }
    serlogF(SER_ERROR, "Too many interrupts");
}

void BasicIoAbstraction::writePort(pinid_t port, uint8_t portVal) {
    // unsupported on mbed at the moment
}

uint8_t BasicIoAbstraction::readPort(pinid_t port) {
    // unsupported on mbed at the moment
    return 0xff;
}

BasicIoAbstraction internalIoAbstraction;

IoAbstractionRef internalDigitalIo() {
    return &internalIoAbstraction;
}

#endif //BUILD_FOR_PICO_CMAKE