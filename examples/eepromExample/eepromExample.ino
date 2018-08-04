/**
 * This sketch is part of IOAbstraction, it shows how to use the EEPROM abstraction,
 * for which you can choose from NoEeprom, AvrEeprom and I2cAt24C based eeproms.
 * Just uncomment the eeprom type you want and comment out the others.
 *
 * This allows any libraries or code you write to work easily across 8 and 32 bit
 * machines by allowing you to decide what type of eeprom you have at compile / runtime.
 *
 * Note that running this sketch WILL WRITE INTO THE SELECTED ROM at the location
 * starting at romStart.
 *
 * It writes a byte, int, double and string to the eeprom and reads them back.
 */

// you always needs this include.
#include <EepromAbstraction.h>

const unsigned int romStart = 800;

// if you are using the I2c based eeprom you need the line below
#include <EepromAbstractionWire.h>

// when you don't want the eeprom writes / reads to do anything.
// comment/ uncomment to select
//NoEeprom anEeprom;

// When you want to use the AVR built in EEPROM support (only available on AVR)
// comment / uncomment to select
//AvrEeprom anEeprom;

// When you want to use an i2c unit use the code below, for I2C roms you also need the AT24CX library which
// is available here: https://github.com/cyberp/AT24Cx
// comment / uncomment to select
I2cAt24Eeprom anEeprom(0x50, PAGESIZE_AT24C128);

char strData[128] = { "this is a really long string that has to be written to eeprom and read back without losing anything at all in the process!"};

void setup() {
	Serial.begin(9600);
	while(!Serial);

  // if you are using the i2c eeprom, you must include this line below, not needed otherwise.
  Wire.begin();

	Serial.println("Eeprom example starting");

	anEeprom.write8(romStart, (byte)42);
	anEeprom.write16(romStart + 1, 0xface);
	anEeprom.write32(romStart + 3, 0xf00dface);
	anEeprom.writeArrayToRom(romStart + 7, (const unsigned char*)strData, sizeof strData);
	Serial.println("Eeprom example written initial values");
}

void loop() {
	Serial.print("Reading back byte: ");
	Serial.println(anEeprom.read8(romStart));

	Serial.print("Reading back word: 0x");
	Serial.println(itoa(anEeprom.read16(romStart + 1), strData, 16));

	Serial.print("Reading back long: 0x");
	Serial.println(ltoa(anEeprom.read32(romStart + 3), strData, 16));

	Serial.print("Reading back array of 20: ");
	anEeprom.readIntoMemArray((unsigned char*)strData, romStart + 7, sizeof strData);
	Serial.println(strData);

	delay(10000);
}
