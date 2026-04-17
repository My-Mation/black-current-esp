#pragma once
// =============================================================
//  keypad_handler.h — 4x4 keypad scanning
//  Maps raw keys to MCQ options (A-D) or numeric digits (0-9)
//  Stores result in gState.pendingKey
// =============================================================
#include <Arduino.h>
#include <Keypad.h>
#include "config.h"

class KeypadHandler {
public:
    void begin();
    void update();   // Call from loop(); writes to gState on keypress

private:
    char _keys[KP_ROWS][KP_COLS] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    byte _rowPins[KP_ROWS] = {KP_R1, KP_R2, KP_R3, KP_R4};
    byte _colPins[KP_COLS] = {KP_C1, KP_C2, KP_C3, KP_C4};
    Keypad _kp{makeKeymap(_keys), _rowPins, _colPins, KP_ROWS, KP_COLS};
};

extern KeypadHandler gKeypad;
