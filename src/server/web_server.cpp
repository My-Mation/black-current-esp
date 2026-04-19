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
    _srv.on("/api/state",         HTTP_OPTIONS,[this](){ _handleCors(); });
    _srv.on("/api/mode",          HTTP_GET,  [this](){ _handleApiMode(); });
    _srv.on("/api/start_test",    HTTP_GET,  [this](){ _handleApiStartTest(); });
    _srv.on("/api/next_question", HTTP_GET,  [this](){ _handleApiNextQuestion(); });
    _srv.on("/api/sync_ans",      HTTP_GET,  [this](){ _handleApiSyncAns(); });
    _srv.on("/api/sync_ans",      HTTP_OPTIONS,[this](){ _handleCors(); });
    _srv.on("/api/reset_voice",   HTTP_GET,  [this](){ _handleApiResetVoice(); });
    _srv.on("/api/load_questions",HTTP_POST, [this](){ _handleApiLoadQuestions(); });
    _srv.on("/api/load_questions",HTTP_OPTIONS,[this](){ _handleCors(); });
    _srv.on("/api/get_questions",   HTTP_GET,  [this](){ _handleApiGetQuestions(); });
    _srv.on("/api/submit",          HTTP_GET,  [this](){ _handleApiSubmit(); });
    _srv.on("/api/submit",          HTTP_OPTIONS,[this](){ _handleCors(); });
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
        "default-src 'self' 'unsafe-inline' https://fonts.googleapis.com https://fonts.gstatic.com; "
        "script-src 'self' 'unsafe-inline' https://cdnjs.cloudflare.com; "
        "media-src 'self' mediastream: blob:; "
        "connect-src 'self' *;");
    
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
    // Use heap instead of stack to avoid crash (Dynamic vs Static)
    DynamicJsonDocument doc(8192); 

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
    doc["index"]     = gState.questionCounter;
    doc["rootIndex"] = gState.rootIndex;
    doc["total"]     = gState.getTotalQuestions();
    doc["isFollowUp"] = gState.interactions[gState.questionCounter].isFollowUp;
    doc["quizId"] = gState.quizId;
    doc["quizTitle"] = gState.quizTitle;
    
    // Send current interaction data for real-time sync
    if (gState.questionCounter >= 0 && gState.questionCounter < 100) {
        if (gState.mode == MODE_MCQ) {
            doc["sel"] = gState.interactions[gState.questionCounter].selectedOption;
        }

        // Include last answer for robust browser sync during transitions
        if (gState.questionCounter > 0) {
            const auto& prev = gState.interactions[gState.questionCounter - 1];
            if (prev.selectedOption != "") doc["prevSel"] = prev.selectedOption;
            if (prev.numericValue != "")   doc["prevNum"] = prev.numericValue;
        }
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

// ------------------------------------------------------------------
//  Build and send a multipart/form-data request to the backend.
//  We manually assemble the body because ESP32's HTTPClient has no
//  built-in multipart builder.
// ------------------------------------------------------------------
static const char* BOUNDARY = "----ESP32Boundary7Ma";

// Helper: append a text field to the multipart body
static void _appendTextField(String& body, const char* fieldName, const String& value) {
    body += "--";
    body += BOUNDARY;
    body += "\r\nContent-Disposition: form-data; name=\"";
    body += fieldName;
    body += "\"\r\n\r\n";
    body += value;
    body += "\r\n";
}

void WebServerHandler::sendResultsToServer() {
    Serial.printf("[SYNC] Starting submission. Free Heap: %d bytes\n", ESP.getFreeHeap());
    
    if (gState.quizId == "" || gState.studentRoll == "") {
        Serial.println("[SYNC] WARNING: Quiz ID or Roll Number is empty! Submission might fail.");
    }

    // ── 1. Build JSON answers array ─────────────────────────────
    // backend expects: [{"answer": "...", "followUpAnswer": "..."}, ...]
    // where index i corresponds to the root question index i.
    DynamicJsonDocument ansDoc(12288);
    JsonArray ansArr = ansDoc.to<JsonArray>();

    // Determine max rootIndex from current quiz if available, else use questionCounter
    int totalRootQuestions = 0;
    if (gState.quizDoc && gState.quizDoc->is<JsonArray>()) {
        totalRootQuestions = gState.quizDoc->as<JsonArray>().size();
    } else {
        // Fallback to searching interactions
        for (int i = 0; i < 100; i++) {
            if (gState.interactions[i].rootIdx >= totalRootQuestions) {
                totalRootQuestions = gState.interactions[i].rootIdx + 1;
            }
        }
    }

    // Initialize array with null objects
    for (int i = 0; i < totalRootQuestions; i++) {
        JsonObject obj = ansArr.createNestedObject();
        obj["answer"] = (char*)nullptr;
        obj["followUpAnswer"] = (char*)nullptr;
    }

    // Populate from interactions
    for (int i = 0; i < 100; i++) {
        const auto& ia = gState.interactions[i];
        if (!ia.answered || ia.rootIdx < 0 || ia.rootIdx >= totalRootQuestions) continue;

        JsonObject obj = ansArr[ia.rootIdx];
        String finalAns = "";
        if (ia.selectedOption != "") finalAns = ia.selectedOption;
        else if (ia.numericValue != "") finalAns = ia.numericValue;

        if (!ia.isFollowUp) {
            if (finalAns != "") obj["answer"] = finalAns;
        } else {
            if (finalAns != "") obj["followUpAnswer"] = finalAns;
        }
    }

    String answersStr;
    serializeJson(ansArr, answersStr);

    // ── 2. Build Multipart Body ────────────────────────────────
    String body = "";
    body.reserve(answersStr.length() + 1024); 
    
    auto appendField = [&](const char* name, const String& val) {
        body += "--"; body += BOUNDARY; body += "\r\n";
        body += "Content-Disposition: form-data; name=\""; body += name; body += "\"\r\n\r\n";
        body += val; body += "\r\n";
    };

    appendField("rollNumber", gState.studentRoll);
    appendField("quizId",     gState.quizId);
    appendField("answers",    answersStr);
    
    body += "--"; body += BOUNDARY; body += "--\r\n";

    Serial.printf("[SYNC] Body built. Size: %d bytes. Free Heap: %d bytes\n", 
                  body.length(), ESP.getFreeHeap());

    // ── 3. Send to Server ───────────────────────────────────────
    int httpCode = 0;
    String responseBody = "";
    const int MAX_RETRIES = 2;

    for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
        HTTPClient http;
        http.begin(SUBMIT_URL);
        http.setTimeout(10000); // 10s timeout
        
        String contentType = "multipart/form-data; boundary=";
        contentType += BOUNDARY;
        http.addHeader("Content-Type", contentType);

        Serial.printf("[SYNC] POST to %s (attempt %d)\n", SUBMIT_URL, attempt + 1);
        httpCode = http.POST((uint8_t*)body.c_str(), body.length());
        
        if (httpCode > 0) {
            // Check size before allocating response string
            int size = http.getSize();
            if (size > 0 && size < 10240) {
                responseBody = http.getString();
            } else {
                Serial.printf("[SYNC] Response too large or unknown: %d bytes\n", size);
            }
            http.end();
            break; 
        }
        
        http.end();
        Serial.printf("[SYNC] Attempt %d failed. Code: %d\n", attempt + 1, httpCode);
        if (attempt < MAX_RETRIES) delay(1000);
    }

    // ── 4. Handle Result ────────────────────────────────────────
    Serial.printf("[SYNC] Final Result: %d. Free Heap: %d bytes\n", httpCode, ESP.getFreeHeap());

    if (httpCode == 201) {
        Serial.println("[SYNC] Submission Success (201).");
        DynamicJsonDocument resDoc(512);
        if (deserializeJson(resDoc, responseBody) == DeserializationError::Ok) {
            int score = resDoc["score"] | -1;
            int total = resDoc["totalQuestions"] | -1;
            if (score >= 0) {
                gOled.showStatus(("Score: " + String(score) + "/" + String(total)).c_str(), "Submit OK");
            } else {
                gOled.showStatus("Submitted!", "Score N/A");
            }
        }
    } else if (httpCode > 0) {
        gOled.showStatus("Submit Warn", ("Code: " + String(httpCode)).c_str());
    } else {
        gOled.showStatus("Submit Failed", "No Response");
    }
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
