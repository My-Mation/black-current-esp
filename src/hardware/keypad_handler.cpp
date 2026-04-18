// =============================================================
//  keypad_handler.cpp — Keypad scanning and state injection
// =============================================================
#include "keypad_handler.h"
#include "buzzer.h"
#include "../server/web_server.h"
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

    // '#' -> Next question navigation / Submit
    if (key == '#') {
        gState.nextQuestion();
        return;
    }

    // '*' -> Previous question navigation (Disabled/Restarts in tree mode)
    if (key == '*') {
        gState.prevQuestion();
        return;
    }

    // 'D' -> Clear numeric input buffer (only in ROLL or NUMERIC modes)
    if (key == 'D' && (gState.mode == MODE_ROLL || gState.mode == MODE_NUM)) {
        gState.numInput = "";
        if (gState.mode == MODE_NUM && gState.questionCounter >= 0) {
            gState.interactions[gState.questionCounter].numericValue = "";
        }
        Serial.println("[KEYPAD] Input: Cleared");
        return;
    }

    // '0' -> Reserved for potential future use (sit idle as requested)
    if (key == '0' && (gState.mode == MODE_IDLE || gState.mode == MODE_READY)) {
         return;
    }

    if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'D')) {
        gState.pendingKey = key;
        gState.keyReady   = true;
        gState.recordFirstInput();
        
        // Track numeric digits regardless of mode if a question is active
        if (key >= '0' && key <= '9' && gState.isActive()) {
            gState.numInput += key;
        }
    }

    // 3. ESP-side internal state tracking (Mode-specific side effects)
    SystemMode m = gState.mode;
    if (m == MODE_ROLL || m == MODE_NUM) {
        if (key >= '0' && key <= '9' && gState.questionCounter >= 0) {
            if (m == MODE_NUM) {
                gState.interactions[gState.questionCounter].numericValue = gState.numInput;
            }
        }
    } else if (m == MODE_MCQ) {
        if (key >= 'A' && key <= 'D') {
            gState.handleMCQ(key);
        }
    }
}
