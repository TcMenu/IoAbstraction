#ifndef TCCLIBS_PICO_ANALOG_DEVICE_H
#define TCCLIBS_PICO_ANALOG_DEVICE_H
#if defined(BUILD_FOR_PICO_CMAKE)

#include "../AnalogDeviceAbstraction.h"
#include <hardware/adc.h>
#include <hardware/pwm.h>

#define ADC_PICO_FIRST_OFFSET 26
#define ADC_PICO_LAST_PIN  30
#define ADC_PICO_VREF 3.3
#define ADC_PICO_BITS 12
#define ADC_PICO_RANGE (1 << 12)
#define ADC_PICO_MAX_VAL (ADC_PICO_RANGE - 1)

class PicoAnalogDevice : public AnalogDevice {
private:
    bool initialised = false;
    float pwmDivider = .8f;
    uint pwmWrap = 4096;
public:
    void setPwmDivider(float pwmDiv, uint wrap) {
        pwmDivider = pwmDiv;
        pwmWrap = wrap;
    }

    void initPin(pinid_t pin, AnalogDirection direction) override;

    int getMaximumRange(AnalogDirection direction, pinid_t pin) override {
        return ADC_PICO_RANGE;
    }

    int getBitDepth(AnalogDirection direction, pinid_t pin) override {
        return ADC_PICO_BITS;
    }

    unsigned int getCurrentValue(pinid_t pin) override;

    float getCurrentFloat(pinid_t pin) override;

    void setCurrentValue(pinid_t pin, unsigned int newValue) override;

    void setCurrentFloat(pinid_t pin, float newValue) override {
        setCurrentValue(pin, uint(newValue * float(ADC_PICO_RANGE)));
    }

    float asVoltageLevel(pinid_t pin) {
        return float(getCurrentValue(pin)) * float(ADC_PICO_VREF / (ADC_PICO_RANGE - 1));
    }
};

PicoAnalogDevice& internalAnalogDevice();

#endif //BUILD_FOR_PICO_CMAKE
#endif //TCCLIBS_PICO_ANALOG_DEVICE_H