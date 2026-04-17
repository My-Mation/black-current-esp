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
    Serial.printf("[KEYPAD] Key pressed: %c (Mode: %d)\n", key, gState.mode);

    // 1. Navigation & Global Controls (Always active if not IDLE)
    if (gState.mode == MODE_IDLE) return; 

    // '#' -> Next question navigation
    if (key == '#') {
        gState.nextQuestion();
        return;
    }

    // '*' -> Previous question navigation
    if (key == '*') {
        gState.prevQuestion();
        return;
    }

    // 'D' -> Clear numeric input buffer (only in ROLL or NUMERIC modes)
    if (key == 'D' && (gState.mode == MODE_ROLL || gState.mode == MODE_NUM)) {
        gState.numInput = "";
        if (gState.mode == MODE_NUM && gState.currentIndex >= 0) {
            gState.interactions[gState.currentIndex].numericValue = "";
        }
        Serial.println("[KEYPAD] Input: Cleared");
        return;
    }

    // '0' -> Submit trigger (only in Done/Ready or at last question - cautiously)
    if (key == '0' && gState.currentIndex == gState.totalQuestions - 1 && 
        gState.mode != MODE_NUM && gState.mode != MODE_ROLL) {
         gState.submitTest();
         return;
    }

    if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'D')) {
        gState.pendingKey = key;
        gState.keyReady   = true;
        gState.recordFirstInput();
        
        // Track numeric digits regardless of mode if a question is active
        // This handles cases where the mode sync is slightly off
        if (key >= '0' && key <= '9' && gState.currentIndex >= 0) {
            gState.numInput += key;
        }
    }

    // 3. ESP-side internal state tracking (Mode-specific side effects)
    SystemMode m = gState.mode;
    if (m == MODE_ROLL || m == MODE_NUM) {
        if (key >= '0' && key <= '9' && gState.currentIndex >= 0) {
            // redundant but safe: keep original logic for specific modes
            if (m == MODE_NUM) {
                gState.interactions[gState.currentIndex].numericValue = gState.numInput;
            }
        }
    } else if (m == MODE_MCQ) {
        if (key >= 'A' && key <= 'D' && gState.currentIndex >= 0) {
            gState.interactions[gState.currentIndex].selectedOption = String(key);
        }
    }
}
