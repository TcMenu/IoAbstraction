#include "ResistiveTouchScreen.h"
#include <SwitchInput.h>

namespace iotouch {

    bool AccelerationHandler::tick() {
        if (mode == WAITING) {
            mode = ACCELERATING;
            ticks = 0;
            accel = 800 / SWITCH_POLL_INTERVAL;
        }
        if (ticks++ > accel) {
            ticks = 0;
            accel = max(minTicks, uint8_t(accel / 2U));
            return true;
        }
        return false;
    }

    void CalibrationHandler::setCalibrationValues(float mnX, float mxX, float mnY, float mxY) {
        minX = mnX;
        minY = mnY;
        maxX = mxX;
        maxY = mxY;
        calibrationOn = true;
    }

    float CalibrationHandler::calibrateX(float rawValue, bool isInverted) const {
        auto x = (calibrationOn) ? ((rawValue - minX) * (1.0F / (maxX - minX))) : rawValue;
        return isInverted ? 1.0F - x : x;
    }

    float CalibrationHandler::calibrateY(float rawValue, bool isInverted) const {
        auto y = (calibrationOn) ? ((rawValue - minY) * (1.0F / (maxY - minY))) : rawValue;
        return isInverted ? 1.0F - y : y;
    }

    void TouchScreenManager::exec() {
        float x;
        float y;
        auto touch = touchInterrogator->internalProcessTouch(&x, &y, rotation, calibrator);
        if (x < 0.0F) x = 0.0F;
        if (y < 0.0F) y = 0.0F;
        // now determine what state we are in, touched, not touched or held.
        auto oldTouchMode = touchMode;
        switch (touch) {
            case NOT_TOUCHED:
                touchMode = NOT_TOUCHED;
                break;
            case TOUCHED:
            case HELD:
                touchMode = (oldTouchMode == TOUCHED || oldTouchMode == HELD) ? HELD : TOUCHED;
                break;
            case TOUCH_DEBOUNCE:
                taskManager.scheduleOnce(5, this, TIME_MILLIS);
                return;
        }

        // we are in a repeated not touch situation, we can slow down the polling slightly now. No update needed
        // even at 1/10th of a second, we'll still wake up pretty quick when they select something.
        if (oldTouchMode == NOT_TOUCHED && touchMode == NOT_TOUCHED) {
            taskManager.scheduleOnce(100, this, TIME_MILLIS);
            accelerationHandler.reset();
            return;
        }

        // only the held  state is subject to acceleration control
        if (touchMode != HELD || usedForScrolling || accelerationHandler.tick()) {
            if (rotation == TouchInterrogator::LANDSCAPE || rotation == TouchInterrogator::LANDSCAPE_INVERTED) {
                sendEvent(y, x, touch, touchMode);
            } else {
                sendEvent(x, y, touch, touchMode);
            }
        }
        taskManager.scheduleOnce(20, this, TIME_MILLIS);
    }

    TouchInterrogator::TouchRotation TouchScreenManager::changeRotation(TouchInterrogator::TouchRotation newRotation) {
        auto oldRotation = rotation;
        rotation = newRotation;
        return oldRotation;
    }

    TouchState ResistiveTouchInterrogator::internalProcessTouch(float *ptrX, float *ptrY, TouchRotation rotation,
                                                                const CalibrationHandler &calibrator) {
        auto *analogDevice = internalAnalogIo();
        auto *device = internalDigitalIo();
        // first we calculate everything in the X dimension.
        analogDevice->initPin(ypPinAdc, DIR_IN);
        device->pinMode(xnPinAdc, OUTPUT);
        device->pinMode(ynPin, INPUT);
        device->pinMode(xpPin, OUTPUT);
        device->digitalWrite(xpPin, HIGH);
        device->digitalWriteS(xnPinAdc, LOW);

        taskManager.yieldForMicros(20);
        float firstSample = analogDevice->getCurrentFloat(ypPinAdc);
        float secondSample = analogDevice->getCurrentFloat(ypPinAdc);

        if (portableFloatAbs(firstSample - secondSample) > 0.007) {
            return TOUCH_DEBOUNCE;
        }
        float x = calibrator.calibrateX((firstSample + secondSample) / 2.0F,
                                        (rotation == LANDSCAPE_INVERTED || rotation == PORTRAIT));

        // now we calculate everything in the Y dimension.
        analogDevice->initPin(xnPinAdc, DIR_IN);
        device->pinMode(xpPin, INPUT);
        device->pinMode(ypPinAdc, OUTPUT);
        device->pinMode(ynPin, OUTPUT);
        device->digitalWrite(ypPinAdc, HIGH);
        device->digitalWriteS(ynPin, LOW);

        taskManager.yieldForMicros(20);
        firstSample = analogDevice->getCurrentFloat(xnPinAdc);
        secondSample = analogDevice->getCurrentFloat(xnPinAdc);

        if (portableFloatAbs(firstSample - secondSample) > 0.007) {
            return TOUCH_DEBOUNCE;
        }
        float y = calibrator.calibrateY((firstSample + secondSample) / 2.0F,
                                        (rotation == LANDSCAPE || rotation == PORTRAIT));

        // and finally the Z dimension
        device->pinMode(xpPin, OUTPUT);
        analogDevice->initPin(ypPinAdc, DIR_IN);
        device->digitalWrite(xpPin, LOW);
        device->digitalWriteS(ynPin, HIGH);

        taskManager.yieldForMicros(20);

        firstSample = analogDevice->getCurrentFloat(xnPinAdc);
        secondSample = analogDevice->getCurrentFloat(ypPinAdc);

        //float touch = ((z2 / z1) * -1.0) * x * resistanceX;
        float touch = 1.0F - (secondSample - firstSample);
        *ptrX = x;
        *ptrY = y;
        return (touch > TOUCH_THRESHOLD) ? TOUCHED : NOT_TOUCHED;
    }

    void ValueStoringResistiveTouchScreen::sendEvent(float locationX, float locationY, float pressure, TouchState touched) {
        lastX = locationX;
        lastY = locationY;
        touchState = touched;
        touchPressure = pressure;
    }
}