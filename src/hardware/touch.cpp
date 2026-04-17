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

    // -------------------------------------------------------
    //  MCQ: single touch = confirm selected option
    // -------------------------------------------------------
    if (m == MODE_MCQ) {
        String sel = gState.interactions[gState.currentIndex].selectedOption;
        if (sel.length() == 0) {
            // Adaptive logic: if we have numeric input instead of an MCQ option,
            // process it as a numeric confirmation to avoid getting stuck.
            if (gState.numInput.length() > 0) {
                Serial.printf("[TOUCH] Adaptive: Confirming NUM (%s) in MCQ mode\n", gState.numInput.c_str());
                gBuzzer.beepConfirm();
                gState.interactions[gState.currentIndex].numericValue = gState.numInput;
                gState.submitCurrent();
                gState.fireTouchEvent(TOUCH_CONFIRMED);
                return;
            }
            // Truly no option chosen — flash but don't advance
            Serial.println("[TOUCH] MCQ – no option selected yet");
            gBuzzer.beepConfirm();
            return;
        }
        gBuzzer.beepConfirm();
        gState.submitCurrent();
        gState.fireTouchEvent(TOUCH_CONFIRMED);
        Serial.printf("[TOUCH] MCQ confirmed: %s\n", sel.c_str());
    }

    // -------------------------------------------------------
    //  NUM: single touch = confirm numeric buffer
    // -------------------------------------------------------
    else if (m == MODE_NUM) {
        if (gState.numInput.length() == 0) {
            Serial.println("[TOUCH] NUM – no digits entered yet");
            gBuzzer.beepConfirm();
            return;
        }
        gBuzzer.beepConfirm();
        gState.interactions[gState.currentIndex].numericValue = gState.numInput;
        gState.submitCurrent();
        gState.fireTouchEvent(TOUCH_CONFIRMED);
        Serial.printf("[TOUCH] NUM confirmed: %s\n", gState.numInput.c_str());
    }

    // -------------------------------------------------------
    //  VOICE: touch sensor = confirm recording (submission)
    // -------------------------------------------------------
    else if (m == MODE_VOICE) {
        gBuzzer.beepConfirm();
        gState.fireTouchEvent(TOUCH_CONFIRMED);
        Serial.println("[TOUCH] VOICE confirm triggered");
    }
    else if (m == MODE_READY && gState.currentIndex == gState.totalQuestions) {
        // Final Submission Trigger
        gBuzzer.beepConfirm();
        gState.submitTest();
        gState.fireTouchEvent(TOUCH_CONFIRMED);
        Serial.println("[TOUCH] Final Test submission triggered");
    }
}
