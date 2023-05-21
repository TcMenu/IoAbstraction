/**
 * This sketch is part of IOAbstraction, it shows how to use the EEPROM abstraction,
 * for which you can choose from many implementations. This example shows  wrapping
 * the standard EEPROM class, but could equally be replaced by any of the others.
 *
 * This allows any libraries or code you write to work easily across 8 and 32 bit
 * machines by allowing you to decide what type of eeprom you have at compile / runtime.
 *
 * Note that running this sketch WILL WRITE INTO THE SELECTED ROM at the location
 * starting at romStart.
 *
 * It writes a byte, int, double and string to the eeprom and reads them back.
 *
 * This is setup by default for ESP32 boards as that's where this class is most
 * heavily used.
 *
 * Documentation and reference:
 *
 * https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
 * https://www.thecoderscorner.com/ref-docs/ioabstraction/html/index.html
 */

// you always needs this include.
#include <EepromAbstraction.h>
#include <ArduinoEEPROMAbstraction.h>
#include <TaskManagerIO.h>
#include <Wire.h>

// where to write into the rom
const int romStart = 300;


// A wrapper that uses the EEPROM class instead. The former is more efficient on AVR.
ArduinoEEPROMAbstraction eepromWrapper(&EEPROM);

const char strData[15] = { "Hello EEPROM"};

void setup() {
    Serial.begin(115200);
    while(!Serial);

    EEPROM.begin(512);

    Serial.println("Eeprom example starting");

    eepromWrapper.write8(romStart, 99);
    eepromWrapper.write16(romStart +1, 0xf00d);
    eepromWrapper.write32(romStart + 3, 0xfade0ff);
    eepromWrapper.writeArrayToRom(romStart + 7, (const unsigned char*)strData, sizeof strData);
    //and if your device needs a commit operation, do it here. For example:
    EEPROM.commit();

    Serial.println("All values written out");
}

void loop() {
    //
    // First we read back using the AVR eeprom directly.
    //

    Serial.print("Reading back byte: ");
    Serial.println(eepromWrapper.read8(romStart));

    Serial.print("Reading back word: 0x");
    Serial.println(eepromWrapper.read16(romStart + 1), HEX);

    Serial.print("Reading back long: 0x");
    Serial.println(eepromWrapper.read32(romStart + 3), HEX);

    Serial.print("Rom Array: ");
    char readBuffer[15];
    eepromWrapper.readIntoMemArray((unsigned char*)readBuffer, romStart + 7, sizeof readBuffer);
    Serial.println(readBuffer);

    delay(10000);
}

