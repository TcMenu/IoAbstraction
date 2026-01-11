/**
 * This sketch is part of IOAbstraction. It shows how to use the EEPROM abstraction,
 * for which you can choose from many implementations. This example demonstrates how
 * to use the ESP32 Preferences API to store data in EEPROM form.
 *
 * This allows any libraries or code you write to work easily across a wide range of
 * hardware by allowing you to decide what type of eeprom you have at compile/runtime.
 *
 * Note that running this sketch WILL WRITE INTO THE SELECTED ROM at the location
 * starting at romStart.
 *
 * It writes a byte, int, double and string to the eeprom and reads them back.
 *
 * This only works on ESP32 boards, with the `preferences` library available.
 *
 * Documentation and reference:
 *
 * https://tcmenu.github.io/documentation/
 * https://tcmenu.github.io/documentation/arduino-libraries/io-abstraction/eeprom-impl-seamless-8-and-32-bit/
 */

#include <EepromAbstraction.h>
#include <esp32/EspPreferencesEeprom.h>
#include <TaskManagerIO.h>
#include <Wire.h>

// We create the preferences wrapper here, simply provide a "namespace", and the size you need.
EspPreferencesEeprom prefsStore("ioaExample", 128);

const char strData[20] = {"Hello ESP32 Prefs"};

void printErrorState() {
    Serial.print("Error occurred? "); Serial.println(prefsStore.hasErrorOccurred() ? "Yes":"No");
}

void setup() {
    Serial.begin(115200);

    Serial.println("Prefs Eeprom example starting");

    // This actually starts the preferences, must be called during setup before use.
    prefsStore.init();

    // we'll only write to the eeprom once at startup, especially as this ROM is probably in FLASH, don't
    // run this test too many times.

    // First we write out the integer types, IE 8, 16 and 32 bit values
    prefsStore.write8(0, 99);
    prefsStore.write16(1, 0xf00d);
    prefsStore.write32(3, 0xfade0ff);

    // now we write an array into the EEPROM
    prefsStore.writeCharArrToRom(7, strData, sizeof strData);

    // This device needs a commit operation to actually save the values.
    prefsStore.commit();

    Serial.println("All values written out");

    printErrorState();
}

void loop() {
    // reload the contents of the ROM into memory
    prefsStore.reload();

    //
    // Now go through reading out each integer data type that we stored during setup.
    //

    Serial.print("Reading back byte: ");
    Serial.println(prefsStore.read8(0));

    Serial.print("Reading back word: 0x");
    Serial.println(prefsStore.read16(1), HEX);

    Serial.print("Reading back long: 0x");
    Serial.println(prefsStore.read32(3), HEX);

    //
    // And lastly we now read back the array that we wrote into there.
    //

    Serial.print("Rom Array: ");
    char readBuffer[20];
    prefsStore.readCharArrIntoMemArray(readBuffer, 7, sizeof readBuffer);
    Serial.println(readBuffer);

    // here we demonstrate how to use the underlying preferences object outside of this wrapper.
    // you simply call `getPreferences()` to get hold of the actual preferences object.
    auto storeSize = prefsStore.getPreferences().getBytesLength(IOA_STORE_KEY);
    Serial.print("Store size: "); Serial.println(storeSize);

    printErrorState();

    // wait a bit between each call.
    delay(10000);
}

