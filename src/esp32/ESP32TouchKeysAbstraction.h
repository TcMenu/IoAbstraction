/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

#ifndef IOABSTRACTION_ESP32TOUCHKEYSABSTRACTION_H
#define IOABSTRACTION_ESP32TOUCHKEYSABSTRACTION_H

#ifndef ESP32
#error "ESP32 specific touch facilities require ESP32"
#endif

#include <IoAbstraction.h>
#include <IoLogging.h>
#include <driver/touch_sensor.h>

/**
 * @file ESP32TouchKeysAbstraction.h
 *
 * Provides a IO abstraction that can be used with switches and other places that supports the ESP32 touch pins, you
 * provide the "pins" in terms of touch pin numbers not GPIO numbers. See the ESP32 documentation for more information
 * about how the touch interface works, the values provided are heavily based on that documentation.
 */

#ifndef DEFAULT_TOUCHKEY_FILTER_FREQ
#define DEFAULT_TOUCHKEY_FILTER_FREQ 10
#endif

// forward reference to interrupt handler.
void esp32TouchKeyInterruptHandler(void* touchAbsAsVoid);
extern volatile int espTouchIntCount;

/**
 * An IoAbstraction tht can work with the touch key sensors on ESP32 boards by using the underlying IDF functions. It
 * enables the capacitive sensors on the touch capable pins and then monitors for values exceeding the threshold. You
 * should read the ESP32 touch sensor guide as this is a fairly thin wrapper over that functionality. It is capable of
 * handling either trigger below or above a threshold value
 */
class ESP32TouchKeysAbstraction : public BasicIoAbstraction {
private:
    uint16_t pinThreshold;
    touch_trigger_mode_t triggerMode = TOUCH_TRIGGER_BELOW;
    bool allOk;
    bool interruptCodeNeeded;
    bool startedUp;
public:
    /**
     * Create an instance of the touch abstraction configuring the IDF touch sensor with the settings you need. Most
     * except the threshold are defaulted.
     * @param defThreshold the value at which the sensor is considered triggered
     * @param highVoltage the high voltage level, see ESP32 touch sensor guide, defaults to TOUCH_HVOLT_2V7
     * @param lowVoltage the low voltage level, see ESP32 touch sensor guide, defaults to TOUCH_LVOLT_0V5
     * @param attenuation the attenuation level to apply, defaults to TOUCH_HVOLT_ATTEN_1V
     */
    explicit ESP32TouchKeysAbstraction(int defThreshold, touch_high_volt_t highVoltage = TOUCH_HVOLT_2V7, touch_low_volt_t lowVoltage = TOUCH_LVOLT_0V5,
                              touch_volt_atten_t attenuation = TOUCH_HVOLT_ATTEN_1V) {
        allOk = touch_pad_init() == ESP_OK;
        serlogF2(SER_IOA_INFO, "touch_pad_init ", allOk);
        touch_pad_set_voltage(highVoltage, lowVoltage, attenuation);
        serlogF2(SER_IOA_DEBUG, "touch_pad set voltage ", allOk);
        interruptCodeNeeded = false;
        startedUp = false;
        pinThreshold = defThreshold;
#ifdef TOUCH_DEBUG_MODE
        taskManager.scheduleFixedRate(5, [] {serdebugF2("intCount=", espTouchIntCount);}, TIME_SECONDS);
#endif
    }

    ~ESP32TouchKeysAbstraction() override = default;

    /**
     * This call ensure that the raw interrupt handler is registered. Call during setup if using interrupt mode.
     */
    void ensureInterruptRegistered() {
        if(!startedUp) {
            startedUp = true;
            touch_pad_filter_start(DEFAULT_TOUCHKEY_FILTER_FREQ);
        }

        if(interruptCodeNeeded) {
            interruptCodeNeeded = false;
            touch_pad_isr_register(esp32TouchKeyInterruptHandler, this);
            allOk = touch_pad_intr_enable() == ESP_OK;
            serlogF2(SER_IOA_INFO, "Enabled interrupts for touch sensor ok=", allOk);
        }
    }

    /**
     * You can call this method to check if any errors occurred during initialisation
     * @return true if correctly created, otherwise false.
     */
    bool isCorrectlyCreated() const { return allOk; }

    /**
     * Set the touch triggering mode, either to be one of `touch_trigger_mode_t` from `touch_sensor_types.h`, this
     * sets the way that triggering is managed. Must be set before setting up any pins (IE before pinMode). Default
     * is BELOW
     * @param theMode one of TOUCH_TRIGGER_BELOW, TOUCH_TRIGGER_ABOVE
     */
    void setTouchTriggerMode(touch_trigger_mode_t theMode) {
        triggerMode = theMode;
    }

    /**
     * Sets the direction, but direction is always input for this abstraction. Must still be called to initialise the
     * touch interface for that pin.
     * @param pin the touch pin number 0..9
     * @param mode always input, ignored
     */
    void pinDirection(pinid_t pin, uint8_t mode) override {
        if(!allOk) return;
        serlogF2(SER_IOA_INFO, "Pin Direction ", pin);
        touch_pad_config((touch_pad_t) pin, pinThreshold);
        touch_pad_set_trigger_mode(triggerMode);
    }

    uint8_t readValue(pinid_t pin) override {
        if(!allOk) {
            serlogF(SER_ERROR, "Can't read touch I/F");
            return 0;
        }

        ensureInterruptRegistered();

        uint16_t val;
        touch_pad_read_filtered((touch_pad_t)pin, &val);
#ifdef TOUCH_DEBUG_MODE
        serlogF3(SER_DEBUG, "Read (Pin,Val) ", pin, val)
#endif

        if(triggerMode == TOUCH_TRIGGER_ABOVE) {
            return val < pinThreshold;
        } else {
            return val > pinThreshold;
        }

    }

    void attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) override {
        interruptCodeNeeded = true;
    }

    bool runLoop() override {
        return allOk;
    }

    //
    // Unimplemented functions
    //
    void writePort(pinid_t pin, uint8_t portVal) override { }
    void writeValue(pinid_t pin, uint8_t value) override { }
    uint8_t readPort(pinid_t pin) override { return 0; }
};

#endif //IOABSTRACTION_ESP32TOUCHKEYSABSTRACTION_H
