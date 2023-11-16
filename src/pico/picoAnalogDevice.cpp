#include "picoAnalogDevice.h"
#include "IoLogging.h"

#if defined(BUILD_FOR_PICO_CMAKE)

PicoAnalogDevice analogDeviceInstance;
AnalogDevice* internalAnalogIo() {
    return &analogDeviceInstance;
}

PicoAnalogDevice& internalAnalogDevice() {
    return analogDeviceInstance;
}


void PicoAnalogDevice::initPin(pinid_t pin, AnalogDirection direction) {
    if(!initialised) {
        initialised = true;
        adc_init();
    }
    if(direction == DIR_IN) {
        if(pin < ADC_PICO_FIRST_OFFSET || pin > ADC_PICO_LAST_PIN) {
            serlogF(SER_ERROR, "Pin outside range");
            return;
        }

        adc_gpio_init(pin);
    } else {
        gpio_set_function(pin, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(pin);
        uint channel_num = pwm_gpio_to_channel(pin);
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, pwmDivider);
        pwm_config_set_wrap(&config, pwmWrap);
        pwm_init(slice_num, &config, true);
        pwm_set_chan_level(slice_num, channel_num, 0);
        pwm_set_enabled(slice_num, true);
    }
}

unsigned int PicoAnalogDevice::getCurrentValue(pinid_t pin) {
    if(pin < ADC_PICO_FIRST_OFFSET || pin > ADC_PICO_LAST_PIN) {
        serlogF(SER_ERROR, "Pin outside range");
        return 0;
    }
    adc_select_input(pin - ADC_PICO_FIRST_OFFSET);
    return adc_read();
}

float PicoAnalogDevice::getCurrentFloat(pinid_t pin) {
    if(pin < ADC_PICO_FIRST_OFFSET || pin > ADC_PICO_LAST_PIN) {
        serlogF(SER_ERROR, "Pin outside range");
        return 0;
    }
    adc_select_input(pin - ADC_PICO_FIRST_OFFSET);
    return float(adc_read()) / ADC_PICO_RANGE;
}

void PicoAnalogDevice::setCurrentValue(pinid_t pin, unsigned int newValue) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint channel_num = pwm_gpio_to_channel(pin);
    if(newValue > ADC_PICO_MAX_VAL) newValue = ADC_PICO_MAX_VAL;
    pwm_set_chan_level(slice_num, channel_num, newValue);
}

#endif // Pico only
