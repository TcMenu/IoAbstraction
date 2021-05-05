/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#include "PlatformDetermination.h"
#include "BasicIoAbstraction.h"

#if defined(IOA_USE_ARDUINO) && defined(ESP32)

IoAbstractionRef arduinoAbstraction = nullptr;
IoAbstractionRef ioUsingArduino() {
    if (arduinoAbstraction == nullptr) {
        arduinoAbstraction = new BasicIoAbstraction();
    }
    return arduinoAbstraction;
}

volatile bool esp32InterruptDriverLoaded = false;

gpio_mode_t toEsp32Mode(uint8_t mode) {
    if(mode == INPUT || mode == INPUT_PULLUP || mode == INPUT_PULLDOWN) return GPIO_MODE_INPUT;
    else return GPIO_MODE_OUTPUT;
}

void BasicIoAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    gpio_config_t config;
    config.pin_bit_mask = 1 << pin;
    config.mode = toEsp32Mode(mode);
    config.pull_up_en = (mode == INPUT_PULLUP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    config.pull_down_en = (mode == INPUT_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);
}

void BasicIoAbstraction::writeValue(pinid_t pin, uint8_t value) {
    gpio_set_level(static_cast<gpio_num_t>(pin), value);
}

uint8_t BasicIoAbstraction::readValue(pinid_t pin) {
    return gpio_get_level(static_cast<gpio_num_t>(pin));
}

void rawCbHandler(void* handler) {
    reinterpret_cast<RawIntHandler>(handler)();
}

void BasicIoAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
    if(!esp32InterruptDriverLoaded) {
        if(ESP_OK == gpio_install_isr_service(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED)) {
            esp32InterruptDriverLoaded = true;
        }
    }
    auto gpioRef = static_cast<gpio_num_t>(pin);
    gpio_set_intr_type(gpioRef, mode == CHANGE ? GPIO_INTR_ANYEDGE : mode == RISING ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(gpioRef, rawCbHandler, (void*)interruptHandler);
    gpio_intr_enable(gpioRef);
}

void BasicIoAbstraction::writePort(pinid_t port, uint8_t portVal) {
    // not directly supported on esp32 functions
}

uint8_t BasicIoAbstraction::readPort(pinid_t port) {
    // not directly supported on esp32 functions
    return 0;
}

#endif
