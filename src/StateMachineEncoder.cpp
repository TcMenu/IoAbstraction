#include "StateMachineEncoder.h"

//
// DO NOT REMOVE THE ISR_ATTR and DRAM_ATTR ANNOTATIONS
//

using namespace tm_internal;

#ifndef STANDARD_DELAY_BETWEEN_CHECKS
#define STANDARD_DELAY_BETWEEN_CHECKS (120L * 1000L * 1000L)
#endif

namespace {
    // 16-element Gray code state transition table
    // Maps 4-bit index ((oldState << 2) | newState) to -1 (CCW), 0 (invalid/no-change), +1 (CW)
#if defined(ESP32) || defined(ESP8266)
    static const DRAM_ATTR int8_t encoderTable[16] = {
#else
    constexpr int8_t encoderTable[16] = {
#endif
        0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };

#if defined(__AVR__)
    class FastPinReader {
        volatile uint8_t* pinReg;
        uint8_t pinMask;

    public:
        explicit FastPinReader(pinid_t pin)
            : pinReg(portInputRegister(digitalPinToPort(pin))),
              pinMask(digitalPinToBitMask(pin)) {}

        inline uint8_t operator*() const { return (*pinReg & pinMask) ? 1 : 0; }
    };
#elif defined(ARDUINO_ARCH_SAMD)
    class FastPinReader {
        PortGroup* samdPort;
        uint32_t pinMask;

    public:
        explicit FastPinReader(pinid_t pin)
            : samdPort(&(PORT->Group[g_APinDescription[pin].ulPort])),
              pinMask(1ul << g_APinDescription[pin].ulPin) {}

        inline uint8_t operator*() const { return (samdPort->IN.reg & pinMask) ? 1 : 0; }
    };
#elif defined(ARDUINO_ARCH_STM32)
    class FastPinReader {
        GPIO_TypeDef* port;
        uint32_t pinMask;

    public:
        explicit FastPinReader(pinid_t pin)
            : port(digitalPinToPort(pin)),
              pinMask(digitalPinToBitMask(pin)) {}

        inline uint8_t operator*() const { return (port->IDR & pinMask) ? 1 : 0; }
    };
#else
    class FastPinReader {
        pinid_t pin;

    public:
        explicit FastPinReader(pinid_t p) : pin(p) {}

        inline ISR_ATTR uint8_t operator*() const {
#if defined(TEENSYDUINO)
            return digitalReadFast(pin);
#elif defined(PICO_SDK_VERSION_MAJOR) || defined(BUILD_FOR_PICO_CMAKE)
            return gpio_get(pin);
#elif defined(ESP32)
            return (pin < 32)
                       ? ((REG_READ(GPIO_IN_REG) >> pin) & 0x1)
                       : ((REG_READ(GPIO_IN1_REG) >> (pin - 32)) & 0x1);
#elif !defined(__MBED__)
            // on STM32Cube and R4 this is highly optimized, but this works everywhere as well.
            return internalDigitalIo()->readValue(pin);
#else
            return 0; // not supported on MBED
#endif
        }
    };
#endif

    class IntClientSideRotaryEncoder : public RotaryEncoder {
    public:
        IntClientSideRotaryEncoder(EncoderCallbackFn cb) : RotaryEncoder(cb) {}
        IntClientSideRotaryEncoder(EncoderListener* ls) : RotaryEncoder(ls) {}
        IntClientSideRotaryEncoder(const IntClientSideRotaryEncoder&) = delete;
        IntClientSideRotaryEncoder(const IntClientSideRotaryEncoder&&) = delete;
        IntClientSideRotaryEncoder& operator=(const IntClientSideRotaryEncoder&) = delete;
    };


    class InterruptSafeStateRotaryEncoder : public BaseEvent {
    private:
        IntClientSideRotaryEncoder encoder;
        FastPinReader pinA;
        FastPinReader pinB;
        int8_t subStepCount = 0;
        uint8_t lastRawState = 0;
        EncoderType encoderType = EncoderType::FULL_CYCLE;
        position_t interruptValue;

    public:
        InterruptSafeStateRotaryEncoder(const pinid_t pinA, const pinid_t pinB, EncoderType encType, EncoderCallbackFn cb)
                : encoder(cb), pinA(pinA), pinB(pinB), encoderType(encType), interruptValue(0) {
            lastRawState = ((*this->pinA << 1) | *this->pinB) & 0x03;

        }

        InterruptSafeStateRotaryEncoder(const pinid_t pinA, const pinid_t pinB, EncoderType encType, EncoderListener* ls)
                : encoder(ls), pinA(pinA), pinB(pinB), encoderType(encType), interruptValue(0) {
            lastRawState = ((*this->pinA << 1) | *this->pinB) & 0x03;
        }

        // do not allow any kind of copying of this hardware specific class.
        InterruptSafeStateRotaryEncoder(const InterruptSafeStateRotaryEncoder&) = delete;
        InterruptSafeStateRotaryEncoder(const InterruptSafeStateRotaryEncoder&&) = delete;
        InterruptSafeStateRotaryEncoder& operator=(const InterruptSafeStateRotaryEncoder&) = delete;

        ISR_ATTR void interruptCallback();

        void exec() override {
            // get the most recent value and make sure the encoder is not currently turning.
            auto intVal = static_cast<int32_t>(atomicRead32(&interruptValue));
            while (!atomicSwap32(&interruptValue, static_cast<uint32_t>(intVal), static_cast<uint32_t>(0))) {
                intVal = static_cast<int32_t>(atomicRead32(&interruptValue));
            }

            // we have to limit the value to the range of a signed byte as that is
            // the maximum range of the increment method.
            intVal = internal_min(127, intVal);
            intVal = internal_max(-127, intVal);

            encoder.increment(static_cast<int8_t>(intVal));
        }

        uint32_t timeOfNextCheck() override {
             return STANDARD_DELAY_BETWEEN_CHECKS;
        }

        RotaryEncoder* getUnderlyingEncoder() {
             return &encoder;
         }
    };

    ISR_ATTR void InterruptSafeStateRotaryEncoder::interruptCallback() {
        const uint8_t a = *pinA;
        const uint8_t b = *pinB;
        const uint8_t newState = ((a << 1) | b) & 0x03;

        // Combine previous 2 bits with current 2 bits into 4-bit index
        const uint8_t index = (lastRawState << 2) | newState;
        lastRawState = newState;

        const int8_t movement = encoderTable[index];
        if (movement != 0) {
            subStepCount += movement;

            const int8_t requiredSubSteps = (encoderType == FULL_CYCLE) ? 4 : ((encoderType == HALF_CYCLE) ? 2 : 1);

            if (subStepCount >= requiredSubSteps) {
                subStepCount = 0;
                auto intVal = static_cast<int32_t>(atomicRead32(&interruptValue));
                while (!atomicSwap32(&interruptValue, static_cast<uint32_t>(intVal), static_cast<uint32_t>(intVal) + 1)) {
                    intVal = static_cast<int32_t>(atomicRead32(&interruptValue));
                }
                markTriggeredAndNotify();
            } else if (subStepCount <= -requiredSubSteps) {
                subStepCount = 0;
                auto intVal = static_cast<int32_t>(atomicRead32(&interruptValue));
                while (!atomicSwap32(&interruptValue, static_cast<uint32_t>(intVal), static_cast<uint32_t>(intVal) - 1)) {
                    intVal = static_cast<int32_t>(atomicRead32(&interruptValue));
                }
                markTriggeredAndNotify();
            }
        }
    }
}

static InterruptSafeStateRotaryEncoder* pEncoderIntSafe = nullptr;

ISR_ATTR void encoderInterruptHandler() {
    if (pEncoderIntSafe != nullptr) {
        pEncoderIntSafe->interruptCallback();
    }
}

bool StateRotaryEncoderBuilder::build() const {
    // make sure configuration is valid
    if (!configurationValid()) {
        serlogF(SER_ERROR, "Encoder config invalid");
        return false;
    }

    // create the encoder with an appropriate callback/listener
    if (this->callback != nullptr) {
        pEncoderIntSafe = new InterruptSafeStateRotaryEncoder(this->pinA, this->pinB, this->encoderType, this->callback);
    } else {
        pEncoderIntSafe = new InterruptSafeStateRotaryEncoder(this->pinA, this->pinB, this->encoderType, this->listener);
    }

    // register both A and B pins as inputs
    auto inputMode = (this->needPullup) ? INPUT_PULLUP : INPUT;
    internalDigitalIo()->pinMode(this->pinA, inputMode);
    internalDigitalIo()->pinMode(this->pinB, inputMode);

    // configure interrupts on the pins if we're in either ALL_PINS or SINGLE_PIN mode.
    // In timer mode the user is responsible for configuring timer interrupts that call encoderInterruptHandler().
    if (interruptMode == EncoderInterruptMode::ALL_PINS) {
        internalDigitalIo()->attachInterrupt(this->pinA, encoderInterruptHandler, CHANGE);
        internalDigitalIo()->attachInterrupt(this->pinB, encoderInterruptHandler, CHANGE);
    } else if (interruptMode == EncoderInterruptMode::SINGLE_PIN) {
        internalDigitalIo()->attachInterrupt(this->interruptPin, encoderInterruptHandler, CHANGE);
    }

    // Lastly, we add the encoder to switches and register it with the task manager
    switches.setEncoder(encoderNum, pEncoderIntSafe->getUnderlyingEncoder());
    taskManager.registerEvent(pEncoderIntSafe);
    return true;
}
