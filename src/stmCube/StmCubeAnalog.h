#ifndef STM32_IOA_ANALOG_H
#define STM32_IOA_ANALOG_H

#include <PlatformDetermination.h>
#include "../AnalogDeviceAbstraction.h"

#ifdef HAL_ADC_Start

class StmCubeAnalogDevice : public AnalogDevice {
private:
    ADC_HandleTypeDef* _hadc;
    uint32_t _resolutionBits;
    uint32_t _maxRawValue;

public:
    // Pass the initialized CubeMX ADC handle (e.g., &hadc1)
    StmCubeAnalogDevice(ADC_HandleTypeDef* hadc, uint32_t resolutionBits = 12)
        : _hadc(hadc), _resolutionBits(resolutionBits) {
        _maxRawValue = (1 << _resolutionBits) - 1;
    }

    virtual ~StmCubeAnalogDevice() = default;

    // Inform the abstraction layer of your maximum bit resolution
    int getMaximumRange(AnalogDirection direction, pinid_t pin) override {
        return _maxRawValue;
    }

    // Configures the pin direction (Not strictly needed for STM32 if handled by CubeMX init)
    void initPin(pinid_t pin, AnalogDirection direction) override {
        // Optional: Ensure GPIO is set to analog mode if not done by CubeMX
    }

    // --- CORE ONE-SHOT READ IMPLEMENTATION ---
    int getCurrentValue(pinid_t pin) override {
        // 1. Select the hardware ADC channel corresponding to this pin ID
        // Note: For multi-channel one-shot, you must reconfigure the channel rank on the fly
        configureChannelForPin(pin);

        // 2. Start conversion
        HAL_ADC_Start(_hadc);

        // 3. Poll for conversion (with an explicit timeout, e.g., 10ms)
        if (HAL_ADC_PollForConversion(_hadc, 10) == HAL_OK) {
            uint32_t rawVal = HAL_ADC_GetValue(_hadc);
            HAL_ADC_Stop(_hadc);
            return (int)rawVal;
        }

        HAL_ADC_Stop(_hadc);
        return 0; // Return zero or fault condition if timeout occurs
    }

    // Returns a normalized value from 0.0 to 1.0
    float getCurrentFloat(pinid_t pin) override {
        return static_cast<float>(getCurrentValue(pin)) / static_cast<float>(_maxRawValue);
    }

    // Pure virtual compliance (can be stubbed out if you only need input reads)
    void setCurrentValue(pinid_t pin, int newValue) override {}

private:
    // Helper to dynamically switch ADC channel structures on a single ADC instance
    void configureChannelForPin(pinid_t pin) {
        ADC_ChannelConfTypeDef sConfig = {0};

        // Simple mapping structure example:
        // You can map pin IDs directly to your micro's channel macros
        if (pin == 0) {
            sConfig.Channel = ADC_CHANNEL_0; // e.g., PA0
        } else if (pin == 1) {
            sConfig.Channel = ADC_CHANNEL_1; // e.g., PA1
        }

        sConfig.Rank = 1; // Highest rank for immediate one-shot sampling
        sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES; // Adjust based on clock tree

        HAL_ADC_ConfigChannel(_hadc, &sConfig);
    }
};
#else
#warning "Skipping analog device, enable at least one ADC to use this class"
#endif
