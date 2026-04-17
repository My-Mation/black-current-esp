// =============================================================
//  web_server.cpp — WiFi setup and all HTTP route handlers
// =============================================================
#include "web_server.h"
#include "html_page.h"
#include "../test_engine/test_state.h"
#include "../hardware/oled_handler.h"
#include <ArduinoJson.h>

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

    _srv.on("/",                  HTTP_GET,  [this](){ _handleRoot(); });
    _srv.on("/api/state",         HTTP_GET,  [this](){ _handleApiState(); });
    _srv.on("/api/mode",          HTTP_GET,  [this](){ _handleApiMode(); });
    _srv.on("/api/start_test",    HTTP_GET,  [this](){ _handleApiStartTest(); });
    _srv.on("/api/next_question", HTTP_GET,  [this](){ _handleApiNextQuestion(); });
    _srv.on("/api/sync_ans",      HTTP_GET,  [this](){ _handleApiSyncAns(); });
    _srv.on("/api/reset_voice",   HTTP_GET,  [this](){ _handleApiResetVoice(); });
    _srv.on("/api/load_questions",HTTP_POST, [this](){ _handleApiLoadQuestions(); });
    _srv.on("/api/load_questions",HTTP_OPTIONS,[this](){ _handleCors(); });
    _srv.on("/api/submit",        HTTP_GET,  [this](){ _handleApiSubmit(); });
    _srv.on("/api/upload_audio",  HTTP_POST, [this](){ _handleApiUploadAudio(); });
    _srv.on("/api/upload_audio",  HTTP_OPTIONS,[this](){ _handleCors(); });
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
    _srv.sendHeader("Cache-Control", "no-cache");
    _srv.send(200, "application/json", json);
}

void WebServerHandler::_sendOk() {
    _srv.sendHeader("Access-Control-Allow-Origin", "*");
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
    // Build minimal JSON state snapshot each poll
    StaticJsonDocument<256> doc;

    // Pending key (consumed — one-shot)
    if (gState.keyReady) {
        char k = gState.consumeKey();
        doc["key"] = String(k);
        Serial.printf("[POLL] Key delivered to browser: %c  Q-idx=%d\n",
                      k, gState.currentIndex);
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
        Serial.printf("[POLL] Touch delivered to browser: %s  Q-idx=%d\n",
                      tStr, gState.currentIndex);
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
    doc["index"] = gState.currentIndex;
    doc["input"] = gState.numInput;

    String out;
    serializeJson(doc, out);
    _sendJson(out);
}


void WebServerHandler::_handleApiMode() {
    // Browser tells ESP what question type is active
    if (_srv.hasArg("m")) {
        String m = _srv.arg("m");
        m.toUpperCase();
        if (m == "MCQ")   gState.mode = MODE_MCQ;
        else if (m == "NUM") gState.mode = MODE_NUM;
        else if (m == "NUMERIC") gState.mode = MODE_NUM;
        else if (m == "VOICE") gState.mode = MODE_VOICE;
        else if (m == "DONE")  gState.mode = MODE_DONE;
        else if (m == "PREV")  gState.prevQuestion();
        else               gState.mode = MODE_IDLE;

        gOled.update();
        Serial.println("[MODE] " + m);
    }
    _sendOk();
}

void WebServerHandler::_handleApiStartTest() {
    gState.startTest();
    gOled.update();
    Serial.println("[TEST] Started");
    _sendOk();
}

void WebServerHandler::_handleApiNextQuestion() {
    gState.nextQuestion();
    Serial.printf("[SERVER] Advanced via API → idx=%d / total=%d\n",
                  gState.currentIndex, gState.totalQuestions);
    gOled.update();
    _sendOk();
}

void WebServerHandler::_handleApiLoadQuestions() {
    if (_srv.method() == HTTP_POST) {
        String body = _srv.arg("plain");
        StaticJsonDocument<512> doc;
        if (!deserializeJson(doc, body)) {
            gState.totalQuestions = doc["count"] | 0;
            gState.mode           = MODE_READY;
            gState.currentIndex   = -1;

            // Store question types
            JsonArray types = doc["types"].as<JsonArray>();
            int idx = 0;
            for (JsonVariant t : types) {
                String ts = t.as<String>();
                if      (ts == "mcq")     gState.questionTypes[idx] = Q_MCQ;
                else if (ts == "numeric") gState.questionTypes[idx] = Q_NUMERIC;
                else if (ts == "voice")   gState.questionTypes[idx] = Q_VOICE;
                else                      gState.questionTypes[idx] = Q_UNKNOWN;
                idx++;
                if (idx >= 50) break;
            }

            gOled.showStatus("Questions Loaded",
                             (String(gState.totalQuestions) + " qns ready").c_str());
            Serial.printf("[QUESTIONS] Loaded %d\n", gState.totalQuestions);
        }
    }
    _sendOk();
}

void WebServerHandler::_handleApiSubmit() {
    gState.mode = MODE_DONE;
    gState.timerRunning = false;
    gOled.showDone(0, gState.totalQuestions);  // score computed browser-side
    Serial.println("[SUBMIT] Test submitted");
    _sendOk();
}

void WebServerHandler::_handleApiSyncAns() {
    if (_srv.hasArg("val") && gState.currentIndex >= 0) {
        String val = _srv.arg("val");
        gState.interactions[gState.currentIndex].selectedOption = val;
        Serial.println("[SYNC] Answer for Q" + String(gState.currentIndex + 1) + " set to " + val);
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
    _srv.send(404, "text/plain", "Not found");
}
