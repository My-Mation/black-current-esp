// =============================================================
//  touch.cpp — Multi-stage touch sensor implementation
// =============================================================
#include "touch.h"
#include "buzzer.h"
#include "../test_engine/test_state.h"

TouchHandler gTouch;

void TouchHandler::begin() {
    pinMode(TOUCH_PIN, INPUT);
    Serial.println("[TOUCH] OK  pin=" + String(TOUCH_PIN) + " (DIGITAL MODE)");
}

void TouchHandler::update() {
    // Using digitalRead instead of touchRead for reliability with TTP223 modules
    int val = digitalRead(TOUCH_PIN);
    bool touched = (val >= TOUCH_THRESHOLD); // TOUCH_THRESHOLD is 1

    // Continuous Debug Print (every 200ms for calibration)
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 200) {
        lastLog = millis();
        Serial.printf("[TOUCH DEBUG] State: %d  Status: %s\n", 
                      val, touched ? "TOUCHED!" : "Idle");
    }

    if (touched && !_prevTouched) {
        unsigned long now = millis();
        if (now - _lastTouchMs >= TOUCH_DEBOUNCE) {
            _lastTouchMs = now;
            _handleTouch();
        }
    }
    _prevTouched = touched;
}

void TouchHandler::_handleTouch() {
    if (!gState.isActive()) {
        Serial.println("[TOUCH] Ignored – no active question");
        return;
    }

    gBuzzer.beepKey();
    SystemMode m = gState.mode;

    if (m == MODE_MCQ) {
        // In MCQ, selecting an option already transitions immediately.
        // Touch can be an alternative confirm if the keypad logic was missed.
        String sel = gState.interactions[gState.questionCounter].selectedOption;
        if (sel.length() > 0) {
            gBuzzer.beepConfirm();
            gState.handleMCQ(sel[0]); 
            gState.fireTouchEvent(TOUCH_CONFIRMED);
        } else {
            Serial.println("[TOUCH] MCQ – no option selected yet");
        }
    }
    else if (m == MODE_NUM || m == MODE_ROLL) {
        if (gState.numInput.length() > 0) {
            gBuzzer.beepConfirm();
            gState.nextQuestion();
            gState.fireTouchEvent(TOUCH_CONFIRMED);
        } else {
            Serial.println("[TOUCH] No digits entered yet");
        }
    }
    else if (m == MODE_VOICE) {
        gBuzzer.beepConfirm();
        gState.nextQuestion();
        gState.fireTouchEvent(TOUCH_CONFIRMED);
        Serial.println("[TOUCH] VOICE confirm triggered");
    }
}
