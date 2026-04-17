// =============================================================
//  touch.cpp — Multi-stage touch sensor implementation
// =============================================================
#include "touch.h"
#include "buzzer.h"
#include "../test_engine/test_state.h"

TouchHandler gTouch;

void TouchHandler::begin() {
    // No pinMode needed — touchRead() uses ADC internally
    Serial.println("[TOUCH] OK  pin=" + String(TOUCH_PIN));
}

void TouchHandler::update() {
    int val = touchRead(TOUCH_PIN);
    bool touched = (val < TOUCH_THRESHOLD);

    // Rising edge detection with debounce
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
            // No option chosen yet — flash but don't advance
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
    //  VOICE: three-stage flow
    //    Stage NONE       → fire REC_START
    //    Stage REC_START  → fire REC_STOP
    //    Stage REC_STOP   → fire CONFIRMED (move on)
    // -------------------------------------------------------
    else if (m == MODE_VOICE) {
        switch (gState.touchStage) {
            case TOUCH_NONE:
                gState.touchStage = TOUCH_REC_START;
                gState.fireTouchEvent(TOUCH_REC_START);
                gState.recordFirstInput();
                Serial.println("[TOUCH] VOICE → REC_START");
                break;

            case TOUCH_REC_START:
                gState.touchStage = TOUCH_REC_STOP;
                gState.fireTouchEvent(TOUCH_REC_STOP);
                gState.interactions[gState.currentIndex].voiceRecorded = true;
                gBuzzer.beepConfirm();
                Serial.println("[TOUCH] VOICE → REC_STOP");
                break;

            case TOUCH_REC_STOP:
                gState.touchStage = TOUCH_CONFIRMED;
                gState.submitCurrent();
                gState.fireTouchEvent(TOUCH_CONFIRMED);
                gBuzzer.beepConfirm();
                Serial.println("[TOUCH] VOICE → CONFIRMED");
                break;

            default:
                break;
        }
    }
}
