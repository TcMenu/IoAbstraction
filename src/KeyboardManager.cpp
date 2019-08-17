
/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 *
 * KeyboadManager.cpp contains the classes needed to deal with editing using matrix keyboards
 */

#include "KeyboardManager.h"
#include "IoLogging.h"

MatrixKeyboardManager::MatrixKeyboardManager() {
    this->ioRef = NULL;
    this->layout = NULL;
    this->listener = NULL;
}

void MatrixKeyboardManager::initialise(IoAbstractionRef ref, KeyboardLayout* layout, KeyboardListener* listener) {
    this->ioRef = ref;
    this->layout = layout;
    this->listener = listener;

    for(int i=0; i<layout->numColumns(); i++) {
        ioDevicePinMode(ioRef, layout->getColPin(i), OUTPUT);
        ioDeviceDigitalWrite(ioRef, layout->getColPin(i), HIGH);
    }
    for(int i=0; i<layout->numRows(); i++) ioDevicePinMode(ioRef, layout->getRowPin(i), INPUT_PULLUP);

    ioDeviceSync(ioRef);

    currentKey = 0;
    taskManager.scheduleFixedRate(KEYBOARD_TASK_MILLIS, this);
}

void MatrixKeyboardManager::setToOuput(int col) {
    for(int i=0; i<layout->numColumns(); i++) {
        ioDeviceDigitalWrite(ioRef, layout->getColPin(i), col != i);
    }
}

void MatrixKeyboardManager::setRepeatKeyMillis(int startAfterMillis, int repeatMillis) {
    repeatStartTicks = startAfterMillis / KEYBOARD_TASK_MILLIS; 
    repeatTicks = repeatMillis / KEYBOARD_TASK_MILLIS; 
}

void MatrixKeyboardManager::exec() {
    if(ioRef == NULL) return;

    char pressThisTime = 0;

    // then we read back the right state
    for(int c=0;c<layout->numColumns();c++) {
        setToOuput(c);        
        ioDeviceSync(ioRef); // first we set the right column low.
        taskManager.yieldForMicros(500); // let things settle while other tasks run.
        ioDeviceSync(ioRef); // then we read the latest row states back
        
        for(int r=0; r<layout->numRows(); r++) {
            if(!ioDeviceDigitalRead(ioRef, layout->getRowPin(r))) {
                pressThisTime = layout->keyFor(r, c);
                serdebugF4("Pressed: ", r, c, (int)pressThisTime);
            }
        }
    }


    // if the key is the same as last time and not zero
    if(pressThisTime == currentKey && pressThisTime) {
        // then we either have finished debouncing or are repeating
        if(keyMode == KEYMODE_DEBOUNCE) {
            keyMode = KEYMODE_PRESSED;
            counter = repeatStartTicks;
            listener->keyPressed(currentKey, false);
        }
        else if(keyMode == KEYMODE_PRESSED) {
            if(counter-- == 0) {
                counter = repeatTicks;
                listener->keyPressed(currentKey, true);
            }
        }
        else keyMode = KEYMODE_DEBOUNCE;
    }
    else {
        // otherwise the keys are not the same, we are either in released 
        // state or debouncing.
        if(keyMode == KEYMODE_PRESSED) {
            keyMode = KEYMODE_NOT_PRESSED;
            counter = 0;
            listener->keyReleased(currentKey);
        }
        if(pressThisTime != 0) keyMode = KEYMODE_DEBOUNCE;
        currentKey = pressThisTime;
    }
}
