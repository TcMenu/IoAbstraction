
#ifndef IOA_RESISTIVETOUCHSCREEN_H
#define IOA_RESISTIVETOUCHSCREEN_H

#include "PlatformDetermination.h"
#include "AnalogDeviceAbstraction.h"
#include <TaskManagerIO.h>

/**
 * @file ResistiveTouchScreen.h A simple resistive touch screen class that converts touch coordinates into a range between
 * floating point values 0.0 and 1.0.
 */

#ifndef TOUCH_THRESHOLD
#define TOUCH_THRESHOLD 0.05F
#endif

namespace iotouch {
    enum AccelerationMode: byte {
        WAITING,
        ACCELERATING,
        NEVER_ACCELERATES
    };

    class AccelerationHandler {
    private:
        uint8_t minTicks;
        uint8_t ticks;
        uint8_t accel;
        AccelerationMode mode;
    public:
        AccelerationHandler(uint8_t minTicks, bool accelerate) : minTicks(minTicks) {
            mode = accelerate ? WAITING : NEVER_ACCELERATES;
        }

        void reset() {
            if(mode == ACCELERATING) mode = WAITING;
        }

        bool tick() {
            if(mode == WAITING) {
                mode = ACCELERATING;
                ticks = 0;
                accel = 800 / SWITCH_POLL_INTERVAL;
            }
            if(ticks++ > accel) {
                ticks = 0;
                accel = max(minTicks, uint8_t(accel / 2U));
                return true;
            }
            return false;
        }
    };

    enum TouchState : byte {
        /** no touch has been detected */
        NOT_TOUCHED,
        /** the display has been touched */
        TOUCHED,
        /** the touch is being dragged or held */
        HELD
    };

#define portableFloatAbs(x) ((x)<0.0F?-(x):(x))

    /**
     * This class handles the basics of a touch screen interface, capturing the values and converting them into a usable
     * form, it is pure abstract and the sendEvent needs implementing with a suitable implemetnation for your needs.
     * It is heavily based on the Adafruit TouchScreen library but modified to work with AnalogDevice so that it can
     * work reliably across a wider range of devices.
     *
     * Important notes
     *
     * * all the GPIOs used must be OUTPUT capable, this matters on some boards such as ESP32
     * * Y+ and X- must be connected to ADC (analog input capable) pins.
     * * it uses taskManager and takes readings at the millisecond interval provided.
     */
    class BaseResistiveTouchScreen : public Executable {
    public:
        enum TouchRotation : byte {
            PORTRAIT,
            PORTRAIT_INVERTED,
            LANDSCAPE,
            LANDSCAPE_INVERTED,
            RAW
        };
    private:
        AnalogDevice *analogDevice;
        IoAbstractionRef device;
        AccelerationHandler accelerationHandler;
        pinid_t xpPin, xnPinAdc, ypPinAdc, ynPin;
        float minValX = 0.0, maxValX = 1.0, minValY = 0.0, maxValY = 1.0;
        enum TouchState touchMode;
        enum TouchRotation rotation;
        bool needsCorrection = false;
        bool usedForScrolling = false;
    public:

        BaseResistiveTouchScreen(AnalogDevice *device, IoAbstractionRef pins, pinid_t xpPin, pinid_t xnPin,
                                 pinid_t ypPin, pinid_t ynPin, TouchRotation rotation)
                : analogDevice(device), device(pins), accelerationHandler(10, true), xpPin(xpPin), xnPinAdc(xnPin),
                  ypPinAdc(ypPin), ynPin(ynPin), touchMode(NOT_TOUCHED), rotation(rotation) {}

        /**
         * Calibrate the actual minimum and maximum values that the touch screen returns. Use the test sketch that
         * writes the touch screen values to the serial port, and also draws on the screen. Get the minimum and
         * maximum values X and Y and then pass them to here. Important note, these are BEFORE rotation.
         *
         * @param xmin the minimum possible value in the X domain
         * @param xmax the maximum possible value in the X domain
         * @param ymin the minimum possible value in the Y domain
         * @param ymax the maximum possible value in the Y domain
         */
        void calibrateMinMaxValues(float xmin, float xmax, float ymin, float ymax) {
            needsCorrection = true;
            minValX = xmin;
            maxValX = xmax;
            minValY = ymin;
            maxValY = ymax;
        }

        TouchRotation changeRotation(TouchRotation newRotation, bool enableCorrection) {
            needsCorrection = enableCorrection;
            auto oldRotation = rotation;
            rotation = newRotation;
            return oldRotation;
        }

        void start() {
            touchMode = NOT_TOUCHED;
            taskManager.execute(this);
        }

        void setUsedForScrolling(bool scrolling) {
            usedForScrolling = scrolling;
        }

        void exec() override {
            // first we calculate everything in the X dimension.
            analogDevice->initPin(ypPinAdc, DIR_IN);
            ioDevicePinMode(device, xnPinAdc, OUTPUT);
            ioDevicePinMode(device, ynPin, INPUT);
            ioDevicePinMode(device, xpPin, OUTPUT);
            ioDeviceDigitalWrite(device, xpPin, HIGH);
            ioDeviceDigitalWriteS(device, xnPinAdc, LOW);

            taskManager.yieldForMicros(20);
            float firstSample = analogDevice->getCurrentFloat(ypPinAdc);
            float secondSample = analogDevice->getCurrentFloat(ypPinAdc);

            if (portableFloatAbs(firstSample - secondSample) > 0.007) {
                taskManager.scheduleOnce(5, this, TIME_MILLIS);
                return;
            }
            float x = ((firstSample + secondSample) / 2.0F);
            if (needsCorrection) x = (x - minValX) * (1.0F / (maxValX - minValX));
            if (rotation == LANDSCAPE_INVERTED || rotation == PORTRAIT) {
                x = 1.0F - x;
            }

            // now we calculate everything in the Y dimension.
            analogDevice->initPin(xnPinAdc, DIR_IN);
            ioDevicePinMode(device, xpPin, INPUT);
            ioDevicePinMode(device, ypPinAdc, OUTPUT);
            ioDevicePinMode(device, ynPin, OUTPUT);
            ioDeviceDigitalWrite(device, ypPinAdc, HIGH);
            ioDeviceDigitalWriteS(device, ynPin, LOW);

            taskManager.yieldForMicros(20);
            firstSample = analogDevice->getCurrentFloat(xnPinAdc);
            secondSample = analogDevice->getCurrentFloat(xnPinAdc);

            if (portableFloatAbs(firstSample - secondSample) > 0.007) {
                taskManager.scheduleOnce(5, this, TIME_MILLIS);
                return;
            }
            float y = ((firstSample + secondSample) / 2.0F);
            if (needsCorrection) y = (y - minValY) * (1.0F / (maxValY - minValY));
            if (rotation == LANDSCAPE || rotation == PORTRAIT) {
                y = 1.0F - y;
            }

            // and finally the Z dimension
            ioDevicePinMode(device, xpPin, OUTPUT);
            analogDevice->initPin(ypPinAdc, DIR_IN);
            ioDeviceDigitalWrite(device, xpPin, LOW);
            ioDeviceDigitalWriteS(device, ynPin, HIGH);

            taskManager.yieldForMicros(20);

            firstSample = analogDevice->getCurrentFloat(xnPinAdc);
            secondSample = analogDevice->getCurrentFloat(ypPinAdc);

            //float touch = ((z2 / z1) * -1.0) * x * resistanceX;
            float touch = 1.0F - (secondSample - firstSample);

            // now determine what state we are in, touched, not touched or held.
            auto oldTouchMode = touchMode;
            if (touch > TOUCH_THRESHOLD) {
                touchMode = (oldTouchMode == TOUCHED || oldTouchMode == HELD) ? HELD : TOUCHED;
            } else {
                touchMode = NOT_TOUCHED;
            }

            // we are in a repeated not touch situation, we can slow down the polling slightly now. No update needed
            // even at 1/10th of a second, we'll still wake up pretty quick when they select something.
            if (oldTouchMode == NOT_TOUCHED && touchMode == NOT_TOUCHED) {
                taskManager.scheduleOnce(100, this, TIME_MILLIS);
                accelerationHandler.reset();
                return;
            }

            // only the held state state is subject to acceleration control
            if(touchMode != HELD || usedForScrolling || accelerationHandler.tick()) {
                if (rotation == LANDSCAPE || rotation == LANDSCAPE_INVERTED) {
                    sendEvent(y, x, touch, touchMode);
                } else {
                    sendEvent(x, y, touch, touchMode);
                }
            }
            taskManager.scheduleOnce(20, this, TIME_MILLIS);
        }

        /**
         * You must create a subclass extends from this and takes the three values converting into an
         * event for processing.
         *
         * @param locationX the location between 0 and 1 in the X domain
         * @param locationY the location between 0 and 1 in the Y domain
         * @param touched if the panel is current touched
         */
        virtual void sendEvent(float locationX, float locationY, float touchPressure, TouchState touched) = 0;
    };

    /**
     * Handles the touch screen interface by storing the latest values for later use. This allows for simple uses of
     * touch screen without creating a sub class, by just getting the latest values as needed.
     */
    class ValueStoringResistiveTouchScreen : public BaseResistiveTouchScreen {
    private:
        float lastX, lastY, touchPressure;
        TouchState touchState;
    public:
        ValueStoringResistiveTouchScreen(AnalogDevice *device, IoAbstractionRef pins, pinid_t xpPin, pinid_t xnPin,
                                         pinid_t ypPin, pinid_t ynPin, TouchRotation rotation) :
                BaseResistiveTouchScreen(device, pins, xpPin, xnPin, ypPin, ynPin, rotation) {}

        void sendEvent(float locationX, float locationY, float pressure, TouchState touched) override {
            lastX = locationX;
            lastY = locationY;
            touchState = touched;
            touchPressure = pressure;
        }

        float getTouchPressure() const {
            return touchPressure;
        }

        float getLastX() const {
            return lastX;
        }

        float getLastY() const {
            return lastY;
        }

        bool isPressed() const {
            return touchState == TOUCHED;
        }

        TouchState getTouchState() const {
            return touchState;
        }
    };
}

#endif //IOA_RESISTIVETOUCHSCREEN_H
