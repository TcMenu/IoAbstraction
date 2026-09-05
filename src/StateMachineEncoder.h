/*
* Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

/**
 * @file StateMachineEncoder.h
 * @brief The statemachine encoder class provides an interrupt or timer based rotary encoder.
 * Fulfills the regular rotary encoder interface in IoAbstraction with an interrupt based rotary encoder. In an ideal
 * world the encoder should have debounce capacitors near the encoder end, but this class should be fairly resistant
 * to noise within reason. NOTE that unlike previous implementations, this does require either both pins are interrupt
 * driven, a separate interrupt pin that is effectively A or B, or a timer interrupt. It is not possible to use this
 * version without an interrupt or timer or over I2C. For this see the other rotary encoder implementations.
 */
#ifndef IOA_STATEMACHINE_ENCODER_H
#define IOA_STATEMACHINE_ENCODER_H

#include "PlatformDetermination.h"
#include "SwitchInput.h"

/**
 * The supported interrupt modes, note that `NONE` is not supported and a form of interrupt must be provided.
 */
enum class EncoderInterruptMode {
    /** The interrupt will be registered on a single pin that is effectively A or B. */
    SINGLE_PIN,
    /** An interrupt will be registered on both A and B pin */
    ALL_PINS,
    /** You are responsible for configuring the timer interrupt (or regular interrupts on pins A and B) */
    TIMER,
    /** This is just the initial case, it is a not valid marker */
    NONE
};

/**
 * When in timer mode, IE you're responsible for configuring the timer interrupt (or regular interrupts on pins A and B)
 * then you must call this method from your timer interrupt handler.
 */
void encoderInterruptHandler();

/**
 * Builder class for configuring a state machine based rotary encoder with various options such as pins, callbacks,
 * listeners, interrupt handling, acceleration modes, and encoder types. Note that unlike other IoAbstraction based
 * rotary encoders, this encoder requires either a dedicated interrupt or a hardware timer to be used. The
 * configuration will not be considered valid unless one of the interrupt/timer modes is set.
 *
 * Interrupt modes that are supported:
 * 1. Interrupt on both pins - both A and B are interrupt capable.
 * 2. Interrupt on single pin - must not be A or B, must be wired such that a change in A or B will trigger the interrupt.
 * 3. Timer mode - uses a hardware timer to generate interrupts. An example would be tie to SYS_TICK (1ms on ARM).
 *
 * If you define the timer mode, it is your responsibility to ensure that the timer is configured to generate interrupts
 * at a rate that is appropriate for your application. For example, on ARM, SYS_TICK is configured to generate interrupts
 * at a rate of 1ms. You could attach to this and it should be fast enough to catch 99.9% of movements.
 */
class StateRotaryEncoderBuilder {
private:
    pinid_t pinA = 2;
    pinid_t pinB = 3;
    pinid_t interruptPin = -1;
    EncoderCallbackFn callback = nullptr;
    EncoderListener* listener = nullptr;
    EncoderInterruptMode interruptMode = EncoderInterruptMode::NONE;
    HWAccelerationMode accelerationMode = HWACCEL_REGULAR;
    EncoderType encoderType = FULL_CYCLE;
    bool needPullup = true;
    uint8_t encoderNum = 0;
public:
    /**
     * Configure the A and B pins of the encoder.
     * @param l_pinA the A pin
     * @param l_pinB the B pin
     */
    StateRotaryEncoderBuilder& withEncoderPins(const pinid_t l_pinA, const pinid_t l_pinB) {
        pinA = l_pinA;
        pinB = l_pinB;
        return *this;
    }

    /**
     * IoAbstraction's Switches allows more than one rotary encoder, this defaults to 0 and for most will not
     * need to be changed.
     * Important note: this class can only provide one encoder, as it is a singleton.
     * @param l_encoderNum the encoder number, defaults to 0
     */
    StateRotaryEncoderBuilder& withEncoderNum(const uint8_t l_encoderNum) {
        this->encoderNum = l_encoderNum;
        return *this;
    }

    /**
     * You can configure whether the encoder pins need pullup input enabled or not. If you do not enable INPUT_PULLUP
     * then you must ensure that the pins are pulled up externally and probably add some capacitance to reduce noise.
     * @param l_needPullup true if the pins need pullup input enabled
     */
    StateRotaryEncoderBuilder& withPullUpInput(const bool l_needPullup) {
        this->needPullup = l_needPullup;
        return *this;
    }

    /**
     * The encoder will use the provided callback function to notify of changes.
     * @param l_callback a callback function matching EncoderCallbackFn
     */
    StateRotaryEncoderBuilder& withCallback(const EncoderCallbackFn l_callback) {
        callback = l_callback;
        return *this;
    }

    /**
     * The encoder will use the provided listener to notify of changes.
     * @param l_listener a listener matching EncoderListener
     */
    StateRotaryEncoderBuilder& withListener(EncoderListener* l_listener) {
        listener = l_listener;
        return *this;
    }

    /**
     * The encoder will use the provided interrupt pin to update its state. This single interrupt pin must be the
     * combination of the A and B pins. I.E. most likely a hardware or of both pins. Only use this when interrupt
     * capable pins are in short supply.
     * @param l_interruptPin the interrupt pin to use
     */
    StateRotaryEncoderBuilder& interruptOnSinglePin(const pinid_t l_interruptPin) {
        interruptMode = EncoderInterruptMode::SINGLE_PIN;
        interruptPin = l_interruptPin;
        return *this;
    }

    /**
     * Enable interrupts on both the A and B pins. This requires that both are interrupt capable pins.
     */
    StateRotaryEncoderBuilder& interruptOnBothPins() {
        interruptMode = EncoderInterruptMode::ALL_PINS;
        return *this;
    }

    /**
     * Indicate to the builder that you are going to use either a hardware timer or manually configure interrupts.
     * If you use a timer, it must be very frequent, I would recommend at least 1000Hz. If you manually configure
     * interrupts it must capture both the A and B pins.
     * In either case you must call encoderInterruptHandler() on every event.
     */
    StateRotaryEncoderBuilder& interruptOnTimer() {
        interruptMode = EncoderInterruptMode::TIMER;
        return *this;
    }

    /**
     * Set the acceleration mode for the encoder, it defaults to regular acceleration.
     * @param l_accelerationMode the acceleration mode to use
     */
    StateRotaryEncoderBuilder& withAccelerationMode(const HWAccelerationMode l_accelerationMode) {
        accelerationMode = l_accelerationMode;
        return *this;
    }

    /**
     * Sets the encoder type, it defaults to FULL_CYCLE. At the moment only full and half encoders are supported.
     * @param l_encoderType the encoder type that you have.
     */
    StateRotaryEncoderBuilder& withEncoderType(const EncoderType l_encoderType) {
        encoderType = l_encoderType;
        return *this;
    }

    /**
     * Checks if the configuration is valid by making sure at least the basic configuration is sensible. It is not
     * guaranteed that this returning true will provide a working encoder.
     * @return true if valid, otherwise false.
     */
    [[nodiscard]] bool configurationValid() const {
        if (listener == nullptr && callback == nullptr) {
            return false; // no notification has been configured
        }

        if (interruptMode == EncoderInterruptMode::SINGLE_PIN && (interruptPin == pinA || interruptPin == pinB || interruptPin < 0)) {
            return false; // The single pin must not be A or B, it should be in hardware as A or B.
        }

        // A and B must not be the same.
        return (pinA != pinB) && interruptMode != EncoderInterruptMode::NONE;
    }

    /**
     * A somewhat internal method used by the setupStateMachineRotaryEncoder function to build the encoder and
     * register it with switches and taskmanager.
     * @return true if the encoder was successfully built, false otherwise
     */
    bool build() const; // NOLINT(*-use-nodiscard) because it is not a getter it is optional error handling.
};

/**
 * Configures a state machine rotary encoder that must either be configured to run through a timer or an interrupt.
 * See the StateRotaryEncoderBuilder for more information about options and configuration.
 * @see StateRotaryEncoderBuilder
 *
 * @param builder a description of how to build the encoder
 * @return true if the encoder was successfully built, false otherwise
 */
inline bool setupStateMachineRotaryEncoder(const StateRotaryEncoderBuilder& builder) {
    return builder.build();
}

#endif
