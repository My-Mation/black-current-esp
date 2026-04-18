#pragma once
// =============================================================
//  test_state.h — Adaptive test state machine
//  Shared between hardware drivers and web server.
// =============================================================

#include <Arduino.h>
#include <ArduinoJson.h>

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
    TOUCH_NONE,
    TOUCH_REC_START,
    TOUCH_REC_STOP,
    TOUCH_CONFIRMED
};

// ---- Interaction record for one question -----------------
struct QuestionInteraction {
    unsigned long startTimeMs;
    unsigned long firstInputTimeMs;
    unsigned long submitTimeMs;
    String selectedOption;
    String numericValue;
    bool voiceRecorded;
    bool answered;
    String questionType;
};

// =============================================================
//  TestState — singleton accessed by all modules
// =============================================================
class TestState {
public:
    // ---- Adaptive Tree Engine -----------------------------
    DynamicJsonDocument* quizDoc = nullptr;
    JsonObject currentNode;
    int questionCounter = 0; 
    int rootIndex = 0;       // tracks progress in the top-level JSON array
    
    // Metadata for backend sync
    String quizId = "";
    String quizTitle = "";

    // ---- History Stack (Back-navigation) -----------------
    JsonObject history[100];
    int        historyTitleIndices[100]; // rootIndex history
    int        historyTop = -1;

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

    // ---- Interactions (History) ---------------------------
    QuestionInteraction interactions[100]; // increased for branching paths

    // ---- Methods ------------------------------------------

    void setReady() {
        mode = MODE_READY;
        studentRoll = "";
        timerRunning = false;
        questionCounter = 0;
        rootIndex = 0;
        historyTop = -1;
        if (quizDoc) {
            if (quizDoc->is<JsonArray>()) {
                currentNode = (*quizDoc)[0];
            } else {
                JsonObject root = quizDoc->as<JsonObject>();
                if (root.containsKey("questions") && root["questions"].is<JsonArray>()) {
                    currentNode = root["questions"][0];
                } else {
                    currentNode = root;
                }
            }
        }
    }

    void resetToIdle() {
        mode = MODE_IDLE;
        studentRoll = "";
        numInput = "";
        quizId = "";
        quizTitle = "";
        timerRunning = false;
        questionCounter = 0;
        rootIndex = 0;
        historyTop = -1;
        if (quizDoc) {
            delete quizDoc;
            quizDoc = nullptr;
        }
        for (int i=0; i<100; i++) clearInteraction(i);
    }

    void loadQuiz(const String& json) {
        if (quizDoc) delete quizDoc;
        // Allocate buffer for the tree. Tree can be nested, so provide enough space.
        quizDoc = new DynamicJsonDocument(8192); 
        DeserializationError err = deserializeJson(*quizDoc, json);
        if (err) {
            Serial.printf("[STATE] JSON Load Error: %s\n", err.c_str());
            delete quizDoc;
            quizDoc = nullptr;
        } else {
            rootIndex = 0;
            quizId = "";
            quizTitle = "";

            if (quizDoc->is<JsonArray>()) {
                currentNode = (*quizDoc)[0];
            } else {
                JsonObject root = quizDoc->as<JsonObject>();
                if (root.containsKey("id")) quizId = root["id"].as<String>();
                if (root.containsKey("quizId")) quizId = root["quizId"].as<String>(); // fallback
                if (root.containsKey("title")) quizTitle = root["title"].as<String>();

                if (root.containsKey("questions") && root["questions"].is<JsonArray>()) {
                    currentNode = root["questions"][0];
                } else {
                    currentNode = root;
                }
            }
            Serial.printf("[STATE] Quiz Loaded: %s (ID: %s)\n", quizTitle.c_str(), quizId.c_str());
        }
    }

    void startTest() {
        mode = MODE_ROLL;
        numInput = "";
        questionCounter = 0;
        rootIndex = 0;
        resetTimer();
        clearInteraction(0);
        Serial.println("[STATE] Starting Roll Input Phase");
    }

    void startActualQuestions() {
        if (quizDoc && !currentNode.isNull()) {
            questionCounter = 0;
            rootIndex = 0;
            // Initialize currentNode to the first question if it's an array
            if (quizDoc->is<JsonArray>()) {
                currentNode = (*quizDoc)[0];
            }
            applyModeForNode(currentNode);
            numInput = ""; // Clear roll number accumulator for actual questions
            resetTimer();
        } else {
            mode = MODE_DONE;
        }
    }

    void handleMCQ(char key) {
        if (mode != MODE_MCQ) return;
        int optIdx = key - 'A';
        JsonArray options = currentNode["options"].as<JsonArray>();
        
        if (optIdx >= 0 && optIdx < (int)options.size()) {
            interactions[questionCounter].selectedOption = String(key);
            interactions[questionCounter].submitTimeMs = millis();
            interactions[questionCounter].answered = true;
            interactions[questionCounter].questionType = "MCQ";
            
            JsonVariant opt = options[optIdx];
            // Check if option is an object with a followUp
            if (opt.is<JsonObject>()) {
                JsonObject optObj = opt.as<JsonObject>();
                if (!optObj["followUp"].isNull()) {
                    Serial.printf("[BRANCH] Moving to MCQ option %c follow-up\n", key);
                    currentNode = optObj["followUp"].as<JsonObject>();
                    advanceToNextNode();
                    return;
                }
            }
            
            // If no follow-up at the option level, check if the question node has a generic follow-up
            if (!currentNode["followUp"].isNull()) {
                Serial.println("[BRANCH] Moving to question-level follow-up");
                currentNode = currentNode["followUp"].as<JsonObject>();
                advanceToNextNode();
            } else {
                // Return to root array
                moveToNextRoot();
            }
        }
    }

    void nextQuestion() {
        if (mode == MODE_ROLL) {
            studentRoll = numInput;
            Serial.println("[STATE] Roll confirmed: " + studentRoll);
            historyTop = -1; // Reset history for actual start
            startActualQuestions();
            return;
        }

        if (mode == MODE_MCQ || mode == MODE_NUM || mode == MODE_VOICE) {
            interactions[questionCounter].submitTimeMs = millis();
            interactions[questionCounter].answered = true;
            
            if (mode == MODE_MCQ) {
                interactions[questionCounter].questionType = "MCQ";
                // If skipping MCQ via nextQuestion (#), we don't change selectedOption (stays "")
            } else if (mode == MODE_NUM) {
                interactions[questionCounter].questionType = "NUM";
                interactions[questionCounter].numericValue = numInput;
            } else {
                interactions[questionCounter].questionType = "VOICE";
            }

            if (!currentNode["followUp"].isNull()) {
                Serial.println("[BRANCH] Moving to question follow-up");
                currentNode = currentNode["followUp"].as<JsonObject>();
                advanceToNextNode();
            } else {
                moveToNextRoot();
            }
        }
    }

    void moveToNextRoot() {
        if (quizDoc && quizDoc->is<JsonArray>()) {
            JsonArray arr = quizDoc->as<JsonArray>();
            rootIndex++;
            if (rootIndex < (int)arr.size()) {
                Serial.printf("[ENGINE] Advancing to root question %d\n", rootIndex);
                currentNode = arr[rootIndex];
                advanceToNextNode();
                return;
            }
        }
        Serial.println("[ENGINE] Reached end of root array");
        finishTest();
    }

    void advanceToNextNode() {
        // Push current node to history BEFORE moving
        if (historyTop < 99) {
            historyTop++;
            history[historyTop] = currentNode;
            historyTitleIndices[historyTop] = rootIndex;
        }

        questionCounter++;
        numInput = "";
        applyModeForNode(currentNode);
        resetTimer();
        if (questionCounter < 100) {
            clearInteraction(questionCounter);
        }
    }

    void finishTest() {
        mode = MODE_DONE;
        timerRunning = false;
        Serial.println("[STATE] Test Complete");
    }

    void prevQuestion() {
        if (historyTop >= 0) {
            Serial.printf("[ENGINE] Back-nav: Popping history (%d)\n", historyTop);
            currentNode = history[historyTop];
            rootIndex = historyTitleIndices[historyTop];
            historyTop--;
            
            if (questionCounter > 0) questionCounter--;
            
            numInput = "";
            applyModeForNode(currentNode);
            resetTimer();
            
            // Note: We don't clear the interaction for the question we just went back to,
            // so if they select something new, it will overwrite.
        } else {
            Serial.println("[STATE] At first question, cannot go back");
        }
    }

    void backspace() {
        if (numInput.length() > 0) {
            numInput.remove(numInput.length() - 1);
            if (isActive() && mode == MODE_NUM) {
                interactions[questionCounter].numericValue = numInput;
            }
        }
    }

    void submitTest() {
        finishTest();
    }

    bool isActive() const {
        return (mode == MODE_MCQ || mode == MODE_NUM || mode == MODE_VOICE || mode == MODE_ROLL);
    }

    bool isDone() const { return mode == MODE_DONE; }

    void recordFirstInput() {
        if (isActive() && questionCounter >= 0 && questionCounter < 100) {
            if (interactions[questionCounter].firstInputTimeMs == 0) {
                interactions[questionCounter].firstInputTimeMs = millis();
            }
        }
    }

    void resetTimer() {
        timerStartMs  = millis();
        timerRunning  = true;
        elapsedSec    = 0;
        touchStage    = TOUCH_NONE;
    }

    void updateTimer() {
        if (!timerRunning) return;
        unsigned int s = (millis() - timerStartMs) / 1000;
        if (s != elapsedSec) elapsedSec = s;
    }

    void fireTouchEvent(TouchStage stage) {
        touchEvent   = true;
        touchPayload = stage;
    }

    char consumeKey() {
        char k = pendingKey;
        pendingKey = 0;
        keyReady   = false;
        return k;
    }

    TouchStage consumeTouch() {
        TouchStage s = touchPayload;
        touchEvent  = false;
        touchPayload = TOUCH_NONE;
        return s;
    }

private:
    void applyModeForNode(JsonObject node) {
        if (node.isNull()) {
            mode = MODE_DONE;
            return;
        }
        String type = node["type"] | "unknown";
        Serial.println("[ENGINE] Current Question Type: " + type);
        
        if (type == "mcq")      mode = MODE_MCQ;
        else if (type == "numeric") mode = MODE_NUM;
        else if (type == "voice")   mode = MODE_VOICE;
        else                        mode = MODE_DONE;

        if (questionCounter < 100) {
            interactions[questionCounter].startTimeMs = millis();
            interactions[questionCounter].questionType = type;
        }
    }

    void clearInteraction(int idx) {
        if (idx < 100) {
            interactions[idx] = {millis(), 0, 0, "", "", false, false, ""};
        }
    }
};

extern TestState gState;
