/*
 * main.cpp — ESP32 Test Station entry point
 *
 * Responsibilities:
 *   - Initialize hardware in the right order (Wire → OLED → TM1637 → Buzzer → WiFi → Server)
 *   - Run each module's update() from the main loop
 *   - Keep setup/loop thin — all logic lives in modules
 *
 * Module dependency graph:
 *   main.cpp
 *    ├── hardware/buzzer.{h,cpp}
 *    ├── hardware/oled_handler.{h,cpp}
 *    ├── hardware/tm1637_handler.{h,cpp}
 *    ├── hardware/keypad_handler.{h,cpp}
 *    ├── hardware/touch.{h,cpp}
 *    ├── server/web_server.{h,cpp}
 *    │    └── server/html_page.h  (PROGMEM)
 *    └── test_engine/test_state.{h,cpp}  (shared singleton)
 */

#include <Arduino.h>

// Hardware modules
#include "hardware/buzzer.h"
#include "hardware/oled_handler.h"
#include "hardware/tm1637_handler.h"
#include "hardware/keypad_handler.h"
#include "hardware/touch.h"

// Server module
#include "server/web_server.h"

// Shared state
#include "test_engine/test_state.h"



// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n========================================");
    Serial.println("   ESP32 Test Station v2.0");
    Serial.println("========================================");

    // 1. Buzzer first — gives audio boot feedback
    gBuzzer.begin();

    // 2. OLED — will show status during WiFi connect
    gOled.begin();
    gOled.showBoot();
    delay(400);
    // 3. TM1637 timer display
    gTm.begin();

    // 4. Keypad 
    gKeypad.begin();

    // 5. Touch sensor 
    gTouch.begin();

    // 6. WiFi — blocks until connected or restarts
    gOled.showConnecting(WIFI_SSID);
    if (gWebServer.beginWiFi()) {
        gWebServer.beginServer();
        // 7. Final UI feedback
        gOled.showReady(gWebServer.getIP());
        gBuzzer.beepBoot();
        Serial.println("[SYSTEM] Active. IP: " + gWebServer.getIP());
    } else {
        gOled.showStatus("WiFi Init Failed", "Check SSID/Pass");
    }

    Serial.println("Open browser: http://" + gWebServer.getIP());
    Serial.println("========================================\n");
}

// ============================================================
void loop() {
    // Shared state updates
    gState.updateTimer();

    // Input polling
    gKeypad.update();
    gTouch.update();

    // Visual updates
    gTm.update();
    gOled.update();

    // Network handling
    gWebServer.update();

    // Audio handling
    gBuzzer.update();

    delay(POLL_PERIOD_MS);
}