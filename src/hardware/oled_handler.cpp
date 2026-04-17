// =============================================================
//  oled_handler.cpp — SSD1306 OLED display implementation
// =============================================================
#include "oled_handler.h"

OledHandler gOled;

bool OledHandler::begin() {
    Wire.begin(OLED_SDA, OLED_SCL);
    _dsp = Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
    if (!_dsp.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] Init FAILED");
        return false;
    }
    _dsp.clearDisplay();
    _dsp.display();
    Serial.println("[OLED] OK");
    return true;
}

// ---- Private helpers ----------------------------------------

void OledHandler::_clear() {
    _dsp.clearDisplay();
    _dsp.setTextColor(SSD1306_WHITE);
}

void OledHandler::_border() {
    _dsp.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
}

void OledHandler::_title(const char* txt) {
    _dsp.setTextSize(1);
    _dsp.setCursor(2, 3);
    _dsp.print(txt);
    _dsp.drawFastHLine(0, 14, OLED_WIDTH, SSD1306_WHITE);
}

void OledHandler::_bigText(const char* txt, int y) {
    _dsp.setTextSize(3);
    // Center horizontally
    int w = strlen(txt) * 18;   // approx 18px per char at size 3
    int x = max(0, (OLED_WIDTH - w) / 2);
    _dsp.setCursor(x, y);
    _dsp.print(txt);
}

// ---- Public methods -----------------------------------------

void OledHandler::showBoot() {
    _clear();
    _border();
    _title("ESP32 TEST STATION");
    _dsp.setTextSize(1);
    _dsp.setCursor(28, 28);
    _dsp.print("Initializing...");
    _dsp.display();
}

void OledHandler::showConnecting(const char* ssid) {
    _clear();
    _border();
    _title("Connecting WiFi");
    _dsp.setTextSize(1);
    _dsp.setCursor(4, 22);
    _dsp.print(ssid);
    _dsp.setCursor(20, 50);
    _dsp.print("Please wait...");
    _dsp.display();
}

void OledHandler::showReady(const String& ip) {
    _clear();
    _border();
    _title("  READY");
    _dsp.setTextSize(1);
    _dsp.setCursor(4, 22);
    _dsp.print("Open browser at:");
    _dsp.setTextSize(1);
    _dsp.setCursor(4, 36);
    _dsp.print(ip);
    _dsp.setCursor(4, 52);
    _dsp.print("Load questions to start");
    _dsp.display();
}

void OledHandler::showMode(SystemMode mode, int qIndex, int total) {
    // Skip redraw if nothing changed
    if (mode == _lastMode && qIndex == _lastIndex) return;
    _lastMode  = mode;
    _lastIndex = qIndex;

    _clear();
    _border();

    // Top: question counter
    if (total > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Q%d / %d", qIndex + 1, total);
        _title(buf);
    } else {
        _title("TEST STATION");
    }

    // Middle: big mode label
    const char* label = "IDLE";
    const char* sub   = "";
    switch (mode) {
        case MODE_MCQ:   label = "MCQ";   sub = "Press A-D then Touch";  break;
        case MODE_NUM:   label = "NUM";   sub = "Keys 0-9  Touch=Send";  break;
        case MODE_VOICE: label = "VOICE"; sub = "Touch: Start/Stop/OK";  break;
        case MODE_DONE:  label = "DONE";  sub = "Submit in browser";     break;
        case MODE_READY: label = "READY"; sub = "Click Test Starter";    break;
        default:         label = "IDLE";  sub = "Load questions first";  break;
    }

    _bigText(label, 18);

    _dsp.setTextSize(1);
    int sw = strlen(sub) * 6;
    _dsp.setCursor(max(0, (OLED_WIDTH - sw) / 2), 52);
    _dsp.print(sub);

    _dsp.display();
}

void OledHandler::showStatus(const char* line1, const char* line2) {
    _clear();
    _border();
    _title("STATUS");
    _dsp.setTextSize(1);
    _dsp.setCursor(4, 20);
    _dsp.print(line1);
    if (line2) {
        _dsp.setCursor(4, 36);
        _dsp.print(line2);
    }
    _dsp.display();
}

void OledHandler::showDone(int correct, int total) {
    _clear();
    _border();
    _title("TEST COMPLETE");
    _dsp.setTextSize(2);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d/%d", correct, total);
    int w = strlen(buf) * 12;
    _dsp.setCursor((OLED_WIDTH - w) / 2, 24);
    _dsp.print(buf);
    _dsp.setTextSize(1);
    _dsp.setCursor(20, 52);
    _dsp.print("Check browser");
    _dsp.display();
}

void OledHandler::update() {
    // Called from loop – only redraws if state changed
    if (!gState.isDone()) {
        showMode(gState.mode, gState.currentIndex, gState.totalQuestions);
    }
}
