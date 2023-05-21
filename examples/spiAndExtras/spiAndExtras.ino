/*
 * IoAbstraction Extras are additional analog and SPI components that are not directly in the core. You can see the
 * intended way to use such a class here and build it into your own application.
 */
#include <extras/Pga2310VolumeControl.h>
#include <SPI.h>

#define VOLUME_CS_PIN 14

SPIWithSettings spiWithSettings(&SPI, VOLUME_CS_PIN);
Pga2310VolumeControl volumeControl(spiWithSettings);

void setup() {
    Serial.begin(115200);
    volumeControl.initPin(0, DIR_OUT);
    volumeControl.initPin(1, DIR_OUT);
}

void loop() {
    volumeControl.setCurrentValue(0, 100);
    volumeControl.setCurrentValue(1, 90);
    serdebugF3("Vol is ", volumeControl.getCurrentValue(0), volumeControl.getCurrentValue(1))
    delay(1000);
    volumeControl.setCurrentFloat(0, 0.1F);
    volumeControl.setCurrentFloat(1, 0.6F);
    serdebugF3("Flt Vol ", volumeControl.getCurrentFloat(0), volumeControl.getCurrentFloat(1))
    delay(1000);
}