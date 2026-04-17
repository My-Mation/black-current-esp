// =============================================================
//  keypad_handler.cpp — Keypad scanning and state injection
// =============================================================
#include "keypad_handler.h"
#include "buzzer.h"
#include "../test_engine/test_state.h"

KeypadHandler gKeypad;

void KeypadHandler::begin() {
    // Keypad library configures GPIO internally
    Serial.println("[KEYPAD] OK");
}

void KeypadHandler::update() {
    char key = _kp.getKey();
    if (!key) return;

    gBuzzer.beepKey();

    // Only inject keys when a test question is active
    if (!gState.isActive()) return;

    SystemMode m = gState.mode;

    // ----- MCQ mode: accept A B C D only -----
    if (m == MODE_MCQ) {
        if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
            gState.pendingKey = key;
            gState.keyReady   = true;
            gState.recordFirstInput();
            // Store selection immediately in the interaction record
            gState.interactions[gState.currentIndex].selectedOption = String(key);
            Serial.printf("[KEYPAD] MCQ select: %c\n", key);
        }
    }

    // ----- Numeric mode: 0-9 accumulate, * clears, # confirms -----
    else if (m == MODE_NUM) {
        if (key >= '0' && key <= '9') {
            gState.numInput += key;
            gState.pendingKey = key;
            gState.keyReady   = true;
            gState.recordFirstInput();
            gState.interactions[gState.currentIndex].numericValue = gState.numInput;
            Serial.printf("[KEYPAD] NUM digit: %c  buffer=%s\n", key, gState.numInput.c_str());
        } else if (key == '*') {
            // Clear numeric input
            gState.numInput = "";
            gState.pendingKey = '*';
            gState.keyReady   = true;
            gState.interactions[gState.currentIndex].numericValue = "";
            Serial.println("[KEYPAD] NUM clear");
        }
        // '#' is handled by touch (confirm), ignore here
    }

    // ----- Voice mode: keypad not used -----
    // else: ignore
}
