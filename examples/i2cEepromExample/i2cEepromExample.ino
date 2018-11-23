/**
 * This sketch is part of IOAbstraction, it shows how to use the EEPROM abstraction,
 * for which you can choose from NoEeprom, AvrEeprom and I2cAt24C based eeproms.
 * This example chooses AvrEeprom, but could equally be replaced by any of the others.
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

// When you want to use the AVR built in EEPROM support (only available on AVR)
// comment / uncomment to select
AvrEeprom anEeprom;

const char strData[15] = { "Hello Eeprom"};

void setup() {
	Serial.begin(9600);
	while(!Serial);

	// if you are using the i2c eeprom, you must include this line below, not needed otherwise.
	Wire.begin();

	Serial.println("Eeprom example starting");

	// now write the values to the rom. 8, 16 and 32 bit
	anEeprom.write8(romStart, (byte)42);
	anEeprom.write16(romStart + 1, 0xface);
	anEeprom.write32(romStart + 3, 0xf00dface);
	
	// lastly write an array to the rom.
	anEeprom.writeArrayToRom(romStart + 7, (const unsigned char*)strData, sizeof strData);

	Serial.println("Eeprom example written initial values");
	
	// we can check if there are any errors writing by calling hasErrorOccurred, for AVR there is never an error.
	// but for i2c variants there may well be.
	Serial.println(anEeprom.hasErrorOccurred() ? "With bus timeouts" : "Successfully");
}

void loop() {

	Serial.print("Reading back byte: ");
	Serial.println(anEeprom.read8(romStart));

	Serial.print("Reading back word: 0x");
	Serial.println(itoa(anEeprom.read16(romStart + 1), readBuffer, 16));

	Serial.print("Reading back long: 0x");
	Serial.println(ltoa(anEeprom.read32(romStart + 3), readBuffer, 16));

	anEeprom.readIntoMemArray((unsigned char*)readBuffer, romStart + 7, sizeof readBuffer);
	Serial.println(readBuffer);

	// finally we'll do hard comparisons against the array, as it's hard to check by hand.
	anEeprom.readIntoMemArray((unsigned char*)readBuffer, romStart + 7, sizeof readBuffer);
	Serial.print("Rom Array");
	Serial.println(readBuffer);

	delay(10000);
}

