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
    MODE_IDLE,    // Switch OFF
    MODE_READY,   // Switch ON, waiting for test
    MODE_ROLL,    // Roll number input
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

    // ---- Student Info -------------------------------------
    String      studentRoll = "";

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

    void setReady() {
        mode = MODE_READY;
        currentIndex = -1;
        studentRoll = "";
        timerRunning = false;
    }

    void resetToIdle() {
        mode = MODE_IDLE;
        currentIndex = -1;
        totalQuestions = 0;
        studentRoll = "";
        numInput = "";
        timerRunning = false;
        for (int i=0; i<50; i++) clearInteraction(i);
    }

    void startTest() {
        // First "question" is roll input
        mode = MODE_ROLL;
        currentIndex = 0;
        numInput = "";
        resetTimer();
        clearInteraction(0);
        Serial.println("[STATE] Starting Roll Input Phase");
    }

    void startActualQuestions() {
        currentIndex = 0;
        if (totalQuestions > 0) {
            applyModeForCurrentQuestion();
            resetTimer();
        } else {
            mode = MODE_DONE;
        }
    }

    void nextQuestion() {
        if (mode == MODE_ROLL) {
            studentRoll = numInput;
            Serial.println("[STATE] Roll confirmed: " + studentRoll);
            startActualQuestions();
            return;
        }

        if (currentIndex < totalQuestions - 1) {
            currentIndex++;
            applyModeForCurrentQuestion();
            resetTimer();
            // Restore answer if exists, otherwise clear
            if (interactions[currentIndex].answered) {
                if (mode == MODE_NUM) numInput = interactions[currentIndex].numericValue;
                else numInput = ""; 
            } else {
                numInput = ""; 
                clearInteraction(currentIndex);
            }
        } else if (currentIndex == totalQuestions - 1) {
            // Move to the final "Ready to Submit" review screen
            currentIndex = totalQuestions;
            mode = MODE_READY; 
            numInput = "";
            Serial.println("[STATE] All questions answered. Entering final review state.");
        }
    }

    void prevQuestion() {
        if (mode == MODE_ROLL) return;
        
        if (currentIndex > 0) {
            currentIndex--;
            applyModeForCurrentQuestion();
            resetTimer();
            // Restore previous input
            if (mode == MODE_NUM) {
                numInput = interactions[currentIndex].numericValue;
            } else {
                numInput = ""; 
            }
        } else if (currentIndex == 0) {
            mode = MODE_ROLL;
            numInput = studentRoll;
            resetTimer();
        }
    }

    void backspace() {
        if (numInput.length() > 0) {
            numInput.remove(numInput.length() - 1);
            if (isActive()) {
                if (mode == MODE_NUM) {
                    interactions[currentIndex].numericValue = numInput;
                }
            }
        }
    }

    void submitTest() {
        mode = MODE_DONE;
        timerRunning = false;
        Serial.println("[STATE] Test Submitted and Completed");
    }

    bool isActive() const {
        if (mode == MODE_READY && currentIndex == totalQuestions) return true;
        return (mode == MODE_MCQ || mode == MODE_NUM || mode == MODE_VOICE || mode == MODE_ROLL);
    }

    bool isDone() const { return mode == MODE_DONE; }

    void recordFirstInput() {
        if (isActive() && currentIndex >= 0 && currentIndex < 50) {
            if (interactions[currentIndex].firstInputTimeMs == 0) {
                interactions[currentIndex].firstInputTimeMs = millis();
            }
        }
    }

    void submitCurrent() {
        if (mode == MODE_ROLL) {
            nextQuestion();
            return;
        }
        if (!isActive()) return;
        interactions[currentIndex].submitTimeMs = millis();
        interactions[currentIndex].answered = true;
        
        // Auto-advance if not the last question
        if (currentIndex < totalQuestions - 1) {
            nextQuestion();
        } else {
            Serial.println("[STATE] Last question confirmed. Waiting for Submit Trigger (0)");
        }
    }

    void resetTimer() {
        timerStartMs  = millis();
        timerRunning  = true;
        elapsedSec    = 0;
        touchStage    = TOUCH_NONE;
        if (mode != MODE_ROLL) {
             // In NUM mode, we might restore answer, so don't always clear numInput here
             // It's handled in next/prev methods
        }
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
