/*
* Copyright (c) 2018 https://www.thecoderscorner.com (Dave Cherry).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */


/**
 * @file EspPreferencesEeprom.h
 * @brief Contains an implementation of the EepromAbstraction that backs onto preferences API. Please note that
 * to use this class, you must make sure the Preferences library is available.
 */

#ifndef IOA_ESPPREFERENCESEEPROM_H
#define IOA_ESPPREFERENCESEEPROM_H

#include <EepromAbstraction.h>
#include <IoLogging.h>
#include <Preferences.h>

// you can define the namespace using
#ifndef IOA_STORE_KEY
#define IOA_STORE_KEY "ioa"
#endif

/**
 * An implementation of EepromAbstraction that backs onto the ESP32 preferences API. It holds an array of the
 * size requested in memory, which is loaded from the preferences at start up, and it can be committed back
 * to storage using the extra `commit` method.
 */
class EspPreferencesEeprom : public EepromAbstraction {
private:
    Preferences prefs;
    uint8_t *menuStore;
    size_t storeSize;
    bool prefsOk = true;
    bool changed = false;
    char romNamespaceStr[20];

public:
    /**
     * Construct the preferences based eeprom, ready to be initialised and used later.
     * @param romNameSpace the namespace to use, up to 20 characters in length
     * @param size the size of the storage area to create.
     */
    EspPreferencesEeprom(const char* romNameSpace, size_t size) {
        menuStore = new uint8_t[size];
        storeSize = size;
        strncpy(romNamespaceStr, romNameSpace, sizeof(romNamespaceStr) - 1);
        romNamespaceStr[sizeof(romNamespaceStr) - 1] = 0;
    }

    /**
     * Initialize the Preferences API with the namespace and size provided in the constructor
     * @return true on success, otherwise false
     */
    bool init() {
        prefs.begin(romNamespaceStr, false);
        // Make sure the store can be loaded and the sizes match. Otherwise, we don't load it.
        if (!prefs.isKey(IOA_STORE_KEY) || prefs.getBytesLength(IOA_STORE_KEY) != storeSize) {
            serlogF2(SER_IOA_INFO, "New prefs ", IOA_STORE_KEY);
            memset(menuStore, 0, storeSize);
        } else {
            reload();
        }
        return prefsOk;
    }

    ~EspPreferencesEeprom() override {
        prefs.end();
        delete[] menuStore;
        prefsOk = false;
    }

    /**
     * Check if an error has occurred during any operation
     * @return true in the event an error was recorded, otherwise false
     */
    bool hasErrorOccurred() override { return !prefsOk; }

    uint8_t read8(EepromPosition position) override {
        if (position >= storeSize || !prefsOk) {
            prefsOk = false;
            return 0;
        }
        return menuStore[position];
    }

    void write8(EepromPosition position, uint8_t val) override {
        if (position >= storeSize || !prefsOk) return;
        menuStore[position] = val;
        changed = true;
    }

    uint16_t read16(EepromPosition position) override {
        return read8(position) | (read8(position + 1) << 8);
    }

    void write16(EepromPosition position, uint16_t val) override {
        write8(position, val & 0xFF);
        write8(position + 1, val >> 8);
        changed = true;
    }

    uint32_t read32(EepromPosition position) override {
        return read16(position) | (read16(position + 2) << 16);

    }

    void write32(EepromPosition position, uint32_t val) override {
        write16(position, val & 0xFFFF);
        write16(position + 2, val >> 16);
        changed = true;
    }

    void readIntoMemArray(uint8_t *memDest, EepromPosition romSrc, uint8_t len) override {
        memcpy(memDest, menuStore + romSrc, len);

    }

    void writeArrayToRom(EepromPosition romDest, const uint8_t *memSrc, uint8_t len) override {
        memcpy(menuStore + romDest, memSrc, len);
        changed = true;
    }

    /**
     * This returns the underlying preferences object for use outside tcMenu/IoAbstraction
     * @return the preferences object for your own use
     */
    Preferences& getPreferences() { return prefs; }

    /**
     * Commit the contents of the preferences back to ROM.
     */
    void commit() {
        if (changed) {
            serlogF2(SER_IOA_INFO, "Prefs written to ", IOA_STORE_KEY);
            prefs.putBytes(IOA_STORE_KEY, menuStore, storeSize);
        }
        changed = false;
    }

    /**
     * Reload the contents of the ROM back into memory, overwriting what's in memory and resetting
     * the OK flag.
     */
    void reload() {
        serlogF2(SER_IOA_INFO, "Load prefs ", IOA_STORE_KEY);
        if (prefs.getBytes(IOA_STORE_KEY, menuStore, storeSize) != storeSize) {
            serlogF3(SER_ERROR, "Prefs load failed ", IOA_STORE_KEY, storeSize);
            prefsOk = false;
        } else {
            prefsOk = true;
        }
    }
};

#endif //IOA_ESPPREFERENCESEEPROM_H
