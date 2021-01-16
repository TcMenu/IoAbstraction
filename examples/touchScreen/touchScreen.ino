/**
 * A simple example of how to use a touch screen with IoAbstraction, it takes the touch screen inputs
 * and reports them to the serial port every few hunderd millis. It's possible to extend the touch
 * base class too and be event-driven, but that's not discussed here.
 */

#include <Arduino.h>
#include <IoAbstraction.h>
#include <ResistiveTouchScreen.h>
#include <Adafruit_ILI9341.h>

#define YPOS_PIN 32
#define XNEG_PIN 33
#define XPOS_PIN 2
#define YNEG_PIN 0
#define MY_CS 22
#define MY_DC 21

ArduinoAnalogDevice analogDevice;
ValueStoringResistiveTouchScreen* touchScreen;
Adafruit_ILI9341 gfx(MY_CS, MY_DC);
int oldX = 0, oldY = 0;

void setup() {
    Serial.begin(115200);
    touchScreen = new ValueStoringResistiveTouchScreen(&analogDevice, internalDigitalIo(), XPOS_PIN, XNEG_PIN, YPOS_PIN, YNEG_PIN, BaseResistiveTouchScreen::PORTRAIT);
    // step 1. run with calibration off and get the actual min and max values if corrections need to be made
    // step 2. put the corrections into the value below, xmin, xmax, ymin, ymax and try the program again.
    touchScreen->calibrateMinMaxValues(0.15F, 0.75F, 0.06F, 0.91F);
    touchScreen->start();

    SPI.begin();
    gfx.begin(20000000);
    gfx.fillScreen(ILI9341_BLACK);

    taskManager.scheduleFixedRate(500, [] {
        serdebugF4("Touch: x, y, touched", touchScreen->getLastX(), touchScreen->getLastY(), touchScreen->getTouchPressure());
        serdebugF2("touchMode: ", touchScreen->getTouchState());

        if(touchScreen->getTouchState() == BaseResistiveTouchScreen::TOUCHED || touchScreen->getTouchState() == BaseResistiveTouchScreen::HELD) {
            gfx.fillCircle(oldX, oldY, 16, ILI9341_BLACK);
            oldX = touchScreen->getLastX() * 240.0F;
            oldY = touchScreen->getLastY() * 320.0F;
            gfx.fillCircle(oldX, oldY, 16, ILI9341_WHITE);
        }
    });
}

void loop() {
    taskManager.runLoop();
}