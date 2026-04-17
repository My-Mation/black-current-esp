#pragma once
// =============================================================
//  test_state.h — Central test state machine
//  Shared between hardware drivers and web server.
//  Owns the canonical system state so all modules read/write
//  the same data without circular dependencies.
// =============================================================

#include <Arduino.h>

// ---- Question type enum ----------------------------------
enum QuestionType { Q_MCQ, Q_NUMERIC, Q_VOICE, Q_UNKNOWN };

// ---- System mode (maps to OLED display) ------------------
enum SystemMode {
    MODE_IDLE,    // No test loaded
    MODE_READY,   // Questions loaded, waiting to start
    MODE_MCQ,     // Active MCQ question
    MODE_NUM,     // Active numeric question
    MODE_VOICE,   // Active voice question
    MODE_DONE     // Test finished
};

// ---- Touch stage (for voice multi-stage) -----------------
enum TouchStage {
    TOUCH_NONE,       // Idle / no active sequence
    TOUCH_REC_START,  // First touch → browser starts recording
    TOUCH_REC_STOP,   // Second touch → browser stops recording
    TOUCH_CONFIRMED   // Third touch → confirmed, move on
};

// ---- Interaction record for one question -----------------
struct QuestionInteraction {
    unsigned long startTimeMs;        // when question was shown
    unsigned long firstInputTimeMs;   // when first keypress/touch happened
    unsigned long submitTimeMs;       // when answer was confirmed
    String selectedOption;            // MCQ: "A"/"B"/"C"/"D"
    String numericValue;              // NUMERIC: accumulated digits
    bool voiceRecorded;               // VOICE: did browser record?
    bool answered;
};

// =============================================================
//  TestState — singleton accessed by all modules
// =============================================================
class TestState {
public:
    // ---- Question metadata set from browser JSON ----------
    int         totalQuestions = 0;
    int         currentIndex   = -1;   // -1 = not started
    QuestionType questionTypes[50];     // max 50 questions

    // ---- Mode & input tracking ----------------------------
    SystemMode  mode      = MODE_IDLE;
    TouchStage  touchStage = TOUCH_NONE;

    // ---- Key & touch events (consumed by browser poll) ----
    char        pendingKey   = 0;
    bool        keyReady     = false;

    bool        touchEvent   = false;
    TouchStage  touchPayload = TOUCH_NONE;

    // ---- Numeric input accumulator ------------------------
    String      numInput = "";

    // ---- Timer --------------------------------------------
    bool         timerRunning  = false;
    unsigned long timerStartMs = 0;
    unsigned int  elapsedSec   = 0;

    // ---- Per-question interaction -------------------------
    QuestionInteraction interactions[50];

    // ---- Methods ------------------------------------------

    void startTest() {
        currentIndex = 0;
        applyModeForCurrentQuestion();
        resetTimer();
        clearInteraction(0);
    }

    void nextQuestion() {
        currentIndex++;
        if (currentIndex >= totalQuestions) {
            mode = MODE_DONE;
            timerRunning = false;
        } else {
            applyModeForCurrentQuestion();
            resetTimer();
            clearInteraction(currentIndex);
        }
    }

    bool isActive() const {
        return currentIndex >= 0 && currentIndex < totalQuestions;
    }

    bool isDone() const { return mode == MODE_DONE; }

    void recordFirstInput() {
        if (isActive() && interactions[currentIndex].firstInputTimeMs == 0) {
            interactions[currentIndex].firstInputTimeMs = millis();
        }
    }

    void submitCurrent() {
        if (!isActive()) return;
        interactions[currentIndex].submitTimeMs = millis();
        interactions[currentIndex].answered = true;
    }

    void resetTimer() {
        timerStartMs  = millis();
        timerRunning  = true;
        elapsedSec    = 0;
        touchStage    = TOUCH_NONE;
        numInput      = "";
    }

    void updateTimer() {
        if (!timerRunning) return;
        unsigned int s = (millis() - timerStartMs) / 1000;
        if (s != elapsedSec) elapsedSec = s;
    }

    // Emit a touch event for the browser to consume
    void fireTouchEvent(TouchStage stage) {
        touchEvent   = true;
        touchPayload = stage;
    }

    // Consume pending key (called from web server poll handler)
    char consumeKey() {
        char k = pendingKey;
        pendingKey = 0;
        keyReady   = false;
        return k;
    }

    // Consume touch event
    TouchStage consumeTouch() {
        TouchStage s = touchPayload;
        touchEvent  = false;
        touchPayload = TOUCH_NONE;
        return s;
    }

private:
    void applyModeForCurrentQuestion() {
        if (currentIndex < 0 || currentIndex >= totalQuestions) return;
        switch (questionTypes[currentIndex]) {
            case Q_MCQ:     mode = MODE_MCQ;   break;
            case Q_NUMERIC: mode = MODE_NUM;   break;
            case Q_VOICE:   mode = MODE_VOICE; break;
            default:        mode = MODE_IDLE;  break;
        }
        interactions[currentIndex].startTimeMs = millis();
    }

    void clearInteraction(int idx) {
        interactions[idx] = {millis(), 0, 0, "", "", false, false};
    }
};

// Global singleton — defined in test_state.cpp, extern'd everywhere
extern TestState gState;
