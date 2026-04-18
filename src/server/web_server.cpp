// =============================================================
//  web_server.cpp — WiFi setup and all HTTP route handlers
// =============================================================
#include "web_server.h"
#include "html_page.h"
#include "../test_engine/test_state.h"
#include "../hardware/oled_handler.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

WebServerHandler gWebServer;

// ===================== WiFi =====================

bool WebServerHandler::beginWiFi() {
    Serial.println("[WIFI] Connecting: " + String(WIFI_SSID));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    for (int i = 0; i < WIFI_TIMEOUT_TRIES; i++) {
        if (WiFi.status() == WL_CONNECTED) break;
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[WIFI] FAILED – restarting in 3s");
        delay(3000);
        ESP.restart();
        return false;
    }
    Serial.println("\n[WIFI] IP: " + WiFi.localIP().toString());
    return true;
}

// ===================== Server setup =====================

void WebServerHandler::beginServer() {
    using namespace std::placeholders;

    // Load persistent settings (Clear if empty)
    _prefs.begin("quiz-cfg", false);
    Serial.println("[SETTINGS] Persistent settings loaded");

    _srv.on("/",                  HTTP_GET,  [this](){ _handleRoot(); });
    _srv.on("/api/state",         HTTP_GET,  [this](){ _handleApiState(); });
    _srv.on("/api/mode",          HTTP_GET,  [this](){ _handleApiMode(); });
    _srv.on("/api/start_test",    HTTP_GET,  [this](){ _handleApiStartTest(); });
    _srv.on("/api/next_question", HTTP_GET,  [this](){ _handleApiNextQuestion(); });
    _srv.on("/api/sync_ans",      HTTP_GET,  [this](){ _handleApiSyncAns(); });
    _srv.on("/api/reset_voice",   HTTP_GET,  [this](){ _handleApiResetVoice(); });
    _srv.on("/api/load_questions",HTTP_POST, [this](){ _handleApiLoadQuestions(); });
    _srv.on("/api/load_questions",HTTP_OPTIONS,[this](){ _handleCors(); });
    _srv.on("/api/get_questions",   HTTP_GET,  [this](){ _handleApiGetQuestions(); });
    _srv.on("/api/submit",          HTTP_GET,  [this](){ _handleApiSubmit(); });
    _srv.on("/api/upload_audio",    HTTP_POST, [this](){ _handleApiUploadAudio(); });
    _srv.on("/api/upload_audio",    HTTP_OPTIONS,[this](){ _handleCors(); });
    _srv.onNotFound([this](){ _handleNotFound(); });

    _srv.begin();
    Serial.println("[SERVER] Started on port " + String(WEB_PORT));
}

void WebServerHandler::update() {
    _srv.handleClient();
}

// ===================== Private helpers =====================

void WebServerHandler::_sendJson(const String& json) {
    _srv.sendHeader("Access-Control-Allow-Origin", "*");
    _srv.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _srv.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    _srv.sendHeader("Cache-Control", "no-cache");
    _srv.send(200, "application/json", json);
}

void WebServerHandler::_sendOk() {
    _srv.sendHeader("Access-Control-Allow-Origin", "*");
    _srv.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _srv.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    _srv.send(200, "text/plain", "OK");
}

// ===================== Route handlers =====================

void WebServerHandler::_handleRoot() {
    // Content-Security-Policy: allow media (mic) from same origin
    _srv.sendHeader("Content-Security-Policy",
        "default-src 'self' 'unsafe-inline' https://fonts.googleapis.com "
        "https://fonts.gstatic.com; media-src 'self' mediastream: blob:;");
    
    // Send in chunks to save RAM and handle large split files
    _srv.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _srv.send(200, "text/html", "");
    _srv.sendContent_P(HTML_PART1);
    _srv.sendContent_P(WEB_CSS);
    _srv.sendContent_P(HTML_PART2);
    _srv.sendContent_P(WEB_JS);
    _srv.sendContent_P(HTML_PART3);
    _srv.sendContent(""); // Final empty chunk to signal end
}

void WebServerHandler::_handleApiState() {
    // Build JSON state snapshot each poll
    StaticJsonDocument<8192> doc;

    // Pending key (consumed — one-shot)
    if (gState.keyReady) {
        char k = gState.consumeKey();
        doc["key"] = String(k);
        Serial.printf("[POLL] Key delivered to browser: %c\n", k);
    } else {
        doc["key"] = "";
    }

    // Touch event (consumed — one-shot)
    if (gState.touchEvent) {
        TouchStage ts = gState.consumeTouch();
        const char* tStr = "NONE";
        switch (ts) {
            case TOUCH_REC_START:  tStr = "REC_START";  break;
            case TOUCH_REC_STOP:   tStr = "REC_STOP";   break;
            case TOUCH_CONFIRMED:  tStr = "CONFIRMED";  break;
            default:               tStr = "NONE";       break;
        }
        doc["touch"] = tStr;
    } else {
        doc["touch"] = "NONE";
    }

    // Timer & mode
    doc["timer"] = gState.elapsedSec;

    const char* modeStr = "IDLE";
    switch (gState.mode) {
        case MODE_ROLL:  modeStr = "ROLL";  break;
        case MODE_MCQ:   modeStr = "MCQ";   break;
        case MODE_NUM:   modeStr = "NUM";   break;
        case MODE_VOICE: modeStr = "VOICE"; break;
        case MODE_READY: modeStr = "READY"; break;
        case MODE_DONE:  modeStr = "DONE";  break;
        default:         modeStr = "IDLE";  break;
    }
    doc["mode"] = modeStr;
    doc["roll"] = gState.studentRoll;
    doc["input"] = gState.numInput;
    doc["index"] = gState.questionCounter;
    doc["rootIndex"] = gState.rootIndex;
    doc["quizId"] = gState.quizId;
    doc["quizTitle"] = gState.quizTitle;
    
    // Send current interaction data for real-time sync
    if (gState.questionCounter >= 0 && gState.questionCounter < 100) {
        doc["sel"] = gState.interactions[gState.questionCounter].selectedOption;
        doc["num"] = gState.interactions[gState.questionCounter].numericValue;
    }
    
    doc["q"] = gState.currentNode;
    doc["fetchStatus"] = getFetchStatus();

    // Send previous interaction for reliable sync
    if (gState.questionCounter > 0) {
        doc["prevSel"] = gState.interactions[gState.questionCounter - 1].selectedOption;
        doc["prevNum"] = gState.interactions[gState.questionCounter - 1].numericValue;
    }

    String out;
    serializeJson(doc, out);
    _sendJson(out);
}


void WebServerHandler::_handleApiMode() {
    // Browser tells ESP what question type is active (fallback/override)
    if (_srv.hasArg("m")) {
        String m = _srv.arg("m");
        m.toUpperCase();
        if (m == "MCQ")        gState.mode = MODE_MCQ;
        else if (m == "NUM")  gState.mode = MODE_NUM;
        else if (m == "VOICE") gState.mode = MODE_VOICE;
        else if (m == "DONE")  gState.mode = MODE_DONE;
        else if (m == "NEXT")  gState.nextQuestion();
        
        gOled.update();
        Serial.println("[MODE_OVERRIDE] " + m);
    }
    _sendOk();
}

void WebServerHandler::_handleApiStartTest() {
    gState.startTest();
    gOled.update();
    Serial.println("[TEST] Started (Adaptive)");
    _sendOk();
}

void WebServerHandler::_handleApiNextQuestion() {
    gState.nextQuestion();
    Serial.printf("[SERVER] Advanced via API → counter=%d\n", gState.questionCounter);
    gOled.update();
    _sendOk();
}

void WebServerHandler::_handleApiLoadQuestions() {
    if (_srv.method() == HTTP_POST) {
        String body = _srv.arg("plain");
        if (_parseQuestionsJson(body)) {
             _sendOk();
        } else {
             _srv.sendHeader("Access-Control-Allow-Origin", "*");
             _srv.send(400, "text/plain", "Invalid JSON");
        }
    }
}

void WebServerHandler::_handleApiGetQuestions() {
    if (_lastJson.length() > 0) {
        _sendJson(_lastJson);
    } else {
        _srv.sendHeader("Access-Control-Allow-Origin", "*");
        _srv.send(404, "text/plain", "No questions loaded yet");
    }
}

bool WebServerHandler::_parseQuestionsJson(const String& body) {
    gState.loadQuiz(body);
    if (gState.quizDoc == nullptr) return false;

    // Reset flow and Auto-Start
    gState.startTest(); 
    gOled.update(); // Immediately show the Roll Number input screen
    Serial.printf("[STATE] Adaptive Engine updated. Quiz: %s (ID: %s)\n", gState.quizTitle.c_str(), gState.quizId.c_str());
    
    return true;
}

void WebServerHandler::_handleApiSubmit() {
    gState.finishTest();
    gOled.showDone();
    Serial.println("[SUBMIT] Test submitted. Attempting background sync...");
    
    // Trigger background send
    sendResultsToServer();
    
    _sendOk();
}

void WebServerHandler::sendResultsToServer() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SYNC] No WiFi, cannot send results.");
        return;
    }

    // Build JSON payload
    DynamicJsonDocument doc(16384); 
    doc["quizId"] = gState.quizId;
    doc["rollNumber"] = gState.studentRoll;
    doc["timestamp"] = millis();

    JsonArray results = doc.createNestedArray("interactions");
    for (int i = 0; i <= gState.questionCounter; i++) {
        if (!gState.interactions[i].answered) continue;
        
        JsonObject obj = results.createNestedObject();
        obj["qIdx"] = i;
        obj["type"] = gState.interactions[i].questionType;
        obj["sel"] = gState.interactions[i].selectedOption;
        obj["num"] = gState.interactions[i].numericValue;
        obj["timeTaken"] = (gState.interactions[i].submitTimeMs - gState.interactions[i].startTimeMs) / 1000;
    }

    String payload;
    serializeJson(doc, payload);

    // Send to backend
    HTTPClient http;
    http.begin(SUBMIT_URL);
    http.addHeader("Content-Type", "application/json");

    Serial.println("[SYNC] Sending results to: " + String(SUBMIT_URL));
    int httpCode = http.POST(payload);

    if (httpCode > 0) {
        Serial.printf("[SYNC] Results sent, code: %d\n", httpCode);
        if (httpCode == 200) {
            gOled.showStatus("Sync Success", "Results Posted");
        } else {
            gOled.showStatus("Sync Error", String("Code: ") + httpCode);
        }
    } else {
        Serial.printf("[SYNC] POST failed: %s\n", http.errorToString(httpCode).c_str());
        gOled.showStatus("Sync Failed", "Server Unreachable");
    }
    http.end();
}

void WebServerHandler::_handleApiSyncAns() {
    if (_srv.hasArg("val") && gState.questionCounter >= 0) {
        String val = _srv.arg("val");
        gState.interactions[gState.questionCounter].selectedOption = val;
        // In adaptive mode, selecting an option via sync (from browser) should also trigger branch
        if (gState.mode == MODE_MCQ) {
            gState.handleMCQ(val[0]);
        }
        Serial.println("[SYNC] Answer for adaptive Q set to " + val);
    }
    _sendOk();
}

void WebServerHandler::_handleApiUploadAudio() {
    // This is a placeholder for the audio upload endpoint.
    // In a real implementation, you might save this to an SD card or stream it.
    // Since ESP32 has limited RAM, we just acknowledge receipt for now.
    Serial.println("[SERVER] Audio upload received (placeholder)");
    _sendOk();
}

void WebServerHandler::_handleApiResetVoice() {
    gState.touchStage = TOUCH_NONE;
    Serial.println("[SERVER] Voice touch stage reset");
    _sendOk();
}

void WebServerHandler::_handleCors() {
    _srv.sendHeader("Access-Control-Allow-Origin", "*");
    _srv.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _srv.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    _srv.send(204);
}

void WebServerHandler::_handleNotFound() {
    _srv.sendHeader("Access-Control-Allow-Origin", "*");
    _srv.send(404, "text/plain", "Not found");
}
