#pragma once
// =============================================================
//  oled_handler.h — SSD1306 OLED display manager
//  Shows: mode name, question progress, IP on boot, status text
// =============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "../test_engine/test_state.h"

class OledHandler {
public:
    bool begin();
    void showBoot();
    void showConnecting(const char* ssid);
    void showReady(const String& ip);
    void showMode(SystemMode mode, int qIndex, int total);
    void showStatus(const char* line1, const char* line2 = nullptr);
    void showDone(int correct, int total);
    void update();   // refresh from gState if needed

private:
    Adafruit_SSD1306 _dsp;
    SystemMode       _lastMode    = MODE_IDLE;
    int              _lastIndex   = -1;
    
    void _clear();
    void _border();
    void _title(const char* txt);
    void _bigText(const char* txt, int y);
};

extern OledHandler gOled;
