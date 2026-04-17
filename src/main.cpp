/*
 * ESP32 Test Station Firmware
 * 
 * Hardware-integrated examination system with web interface.
 * Supports MCQ (keypad A-D), Numeric (keypad 0-9), and Voice (touch sensor) questions.
 * 
 * Hardware:
 *   - OLED SSD1306 128x64 (I2C: SDA=21, SCL=22)
 *   - TM1637 7-Segment Display (CLK=18, DIO=19)
 *   - 4x4 Keypad HX543 (Rows: 13,12,14,27 / Cols: 26,25,33,32)
 *   - Buzzer (GPIO 5)
 *   - Touch Sensor (GPIO 4)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1637Display.h>
#include <Keypad.h>
#include <ArduinoJson.h>
#include "config.h"
#include "web_page.h"

// ===================== HARDWARE OBJECTS =====================

// OLED Display
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// TM1637 Display
TM1637Display tm1637(TM1637_CLK, TM1637_DIO);

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {KP_ROW1, KP_ROW2, KP_ROW3, KP_ROW4};
byte colPins[COLS] = {KP_COL1, KP_COL2, KP_COL3, KP_COL4};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Web Server
WebServer server(WEB_PORT);

// ===================== STATE VARIABLES =====================

// Current mode: "IDLE", "MCQ", "NUM", "VOICE", "DONE"
String currentMode = "IDLE";

// Last key pressed (consumed by browser poll)
volatile char lastKey = 0;
bool keyAvailable = false;

// Touch sensor state
volatile bool touchEvent = false;
unsigned long lastTouchTime = 0;

// Timer
bool timerRunning = false;
unsigned long timerStartMs = 0;
unsigned int currentTimerSec = 0;

// Question tracking
int totalQuestions = 0;
int currentQuestion = 0;
bool testActive = false;

// Buzzer state
bool buzzerActive = false;
unsigned long buzzerEndMs = 0;

// ===================== OLED DISPLAY FUNCTIONS =====================

void oledShowMode(const String& mode) {
  oled.clearDisplay();
  
  // Draw border
  oled.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
  oled.drawRect(2, 2, OLED_WIDTH - 4, OLED_HEIGHT - 4, SSD1306_WHITE);
  
  // Title
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(20, 8);
  oled.print("TEST STATION");
  
  // Horizontal line
  oled.drawFastHLine(4, 20, OLED_WIDTH - 8, SSD1306_WHITE);
  
  // Mode - large text
  oled.setTextSize(2);
  int modeWidth = mode.length() * 12;
  int modeX = (OLED_WIDTH - modeWidth) / 2;
  oled.setCursor(modeX, 28);
  oled.print(mode);
  
  // Question counter at bottom
  if (testActive && totalQuestions > 0) {
    oled.setTextSize(1);
    String qStr = "Q" + String(currentQuestion + 1) + "/" + String(totalQuestions);
    int qWidth = qStr.length() * 6;
    oled.setCursor((OLED_WIDTH - qWidth) / 2, 52);
    oled.print(qStr);
  }
  
  oled.display();
}

void oledShowWelcome() {
  oled.clearDisplay();
  oled.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
  
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(20, 8);
  oled.print("TEST STATION");
  
  oled.drawFastHLine(4, 20, OLED_WIDTH - 8, SSD1306_WHITE);
  
  oled.setTextSize(1);
  oled.setCursor(22, 28);
  oled.print("WiFi Connected");
  
  oled.setCursor(10, 42);
  oled.print(WiFi.localIP().toString());
  
  oled.setCursor(18, 54);
  oled.print("Ready to test");
  
  oled.display();
}

void oledShowConnecting() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 20);
  oled.print("Connecting WiFi...");
  oled.setCursor(10, 35);
  oled.print(WIFI_SSID);
  oled.display();
}

// ===================== TM1637 DISPLAY FUNCTIONS =====================

void tm1637ShowTime(int seconds) {
  int mins = seconds / 60;
  int secs = seconds % 60;
  
  uint8_t data[4];
  data[0] = tm1637.encodeDigit(mins / 10);
  data[1] = tm1637.encodeDigit(mins % 10) | 0x80; // colon
  data[2] = tm1637.encodeDigit(secs / 10);
  data[3] = tm1637.encodeDigit(secs % 10);
  
  tm1637.setSegments(data);
}

void tm1637ShowDashes() {
  uint8_t dash[] = {SEG_G, SEG_G, SEG_G, SEG_G};
  tm1637.setSegments(dash);
}

void tm1637ShowDone() {
  uint8_t data[] = {
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,  // d
    SEG_C | SEG_D | SEG_E | SEG_G,           // o
    SEG_C | SEG_E | SEG_G,                    // n
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G    // E
  };
  tm1637.setSegments(data);
}

// ===================== BUZZER FUNCTIONS =====================

void buzzerBeep(unsigned int durationMs) {
  tone(BUZZER_PIN, BUZZER_FREQ, durationMs);
  buzzerActive = true;
  buzzerEndMs = millis() + durationMs;
}

void buzzerUpdate() {
  if (buzzerActive && millis() >= buzzerEndMs) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}

// ===================== TIMER FUNCTIONS =====================

void startTimer() {
  timerStartMs = millis();
  timerRunning = true;
  currentTimerSec = 0;
}

void stopTimer() {
  timerRunning = false;
}

void resetTimer() {
  timerStartMs = millis();
  currentTimerSec = 0;
  timerRunning = true;
  tm1637ShowTime(0);
}

void updateTimer() {
  if (!timerRunning) return;
  
  unsigned int elapsed = (millis() - timerStartMs) / 1000;
  if (elapsed != currentTimerSec) {
    currentTimerSec = elapsed;
    tm1637ShowTime(currentTimerSec);
  }
}

// ===================== TOUCH SENSOR =====================

void IRAM_ATTR onTouchISR() {
  // Handled in polling to avoid ISR complexity
}

void checkTouch() {
  int touchVal = touchRead(TOUCH_PIN);
  
  if (touchVal < TOUCH_THRESHOLD) {
    unsigned long now = millis();
    if (now - lastTouchTime > TOUCH_DEBOUNCE_MS) {
      lastTouchTime = now;
      touchEvent = true;
      buzzerBeep(BUZZER_BEEP_MS);
      Serial.println("[TOUCH] Touch detected!");
    }
  }
}

// ===================== KEYPAD =====================

void checkKeypad() {
  char key = keypad.getKey();
  if (key) {
    lastKey = key;
    keyAvailable = true;
    buzzerBeep(BUZZER_BEEP_MS);
    Serial.printf("[KEYPAD] Key: %c\n", key);
  }
}

// ===================== WEB SERVER HANDLERS =====================

// Serve main page
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

// API: Get current state (polled by browser)
void handleAPIState() {
  StaticJsonDocument<256> doc;
  
  // Key press (consumed after read)
  if (keyAvailable) {
    doc["key"] = String(lastKey);
    keyAvailable = false;
    lastKey = 0;
  } else {
    doc["key"] = "";
  }
  
  // Touch event (consumed after read)
  doc["touch_event"] = touchEvent;
  if (touchEvent) {
    touchEvent = false;
  }
  
  // Timer
  doc["timer"] = currentTimerSec;
  
  // Mode
  doc["mode"] = currentMode;
  
  // Test state
  doc["test_active"] = testActive;
  doc["current_q"] = currentQuestion;
  doc["total_q"] = totalQuestions;
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

// API: Set mode (called by browser when question changes)
void handleAPIMode() {
  if (server.hasArg("m")) {
    currentMode = server.arg("m");
    oledShowMode(currentMode);
    Serial.printf("[MODE] Changed to: %s\n", currentMode.c_str());
    
    if (currentMode == "DONE") {
      stopTimer();
      tm1637ShowDone();
      testActive = false;
    }
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

// API: Start test
void handleAPIStartTest() {
  testActive = true;
  currentQuestion = 0;
  currentMode = "IDLE";
  startTimer();
  oledShowMode("START");
  Serial.println("[TEST] Test started!");
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

// API: Reset timer (called when moving to next question)
void handleAPIResetTimer() {
  resetTimer();
  currentQuestion++;
  Serial.printf("[TIMER] Reset. Question: %d\n", currentQuestion);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

// API: Load questions info
void handleAPILoadQuestions() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, body);
    
    if (!err) {
      totalQuestions = doc["count"] | 0;
      currentQuestion = 0;
      Serial.printf("[QUESTIONS] Loaded %d questions\n", totalQuestions);
      
      oled.clearDisplay();
      oled.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
      oled.setTextSize(1);
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(15, 16);
      oled.print("Questions Loaded");
      oled.setTextSize(2);
      oled.setCursor(48, 34);
      oled.print(totalQuestions);
      oled.display();
    }
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

// Handle CORS preflight
void handleCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

// 404
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ===================== WIFI =====================

void connectWiFi() {
  Serial.println("\n[WIFI] Connecting to " + String(WIFI_SSID));
  oledShowConnecting();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Animate on TM1637
    uint8_t anim[4] = {0, 0, 0, 0};
    anim[attempts % 4] = SEG_G;
    tm1637.setSegments(anim);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
    oledShowWelcome();
    tm1637ShowDashes();
    buzzerBeep(200);
  } else {
    Serial.println("\n[WIFI] Connection failed!");
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(10, 25);
    oled.print("WiFi FAILED!");
    oled.setCursor(10, 40);
    oled.print("Restarting...");
    oled.display();
    delay(3000);
    ESP.restart();
  }
}

// ===================== SETUP =====================

void setup() {
  Serial.begin(115200);
  Serial.println("\n============================================");
  Serial.println("   ESP32 Test Station - Starting Up");
  Serial.println("============================================\n");
  
  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] Allocation failed!");
  } else {
    Serial.println("[OLED] Initialized OK");
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(10, 25);
    oled.print("Booting...");
    oled.display();
  }
  
  // Initialize TM1637
  tm1637.setBrightness(5);
  tm1637ShowDashes();
  Serial.println("[TM1637] Initialized OK");
  
  // Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("[BUZZER] Initialized OK");
  
  // Boot beep
  buzzerBeep(100);
  delay(150);
  buzzerBeep(100);
  
  // Initialize Touch
  Serial.println("[TOUCH] Initialized OK");
  
  // Connect WiFi
  connectWiFi();
  
  // Setup Web Server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleAPIState);
  server.on("/api/mode", HTTP_GET, handleAPIMode);
  server.on("/api/start_test", HTTP_GET, handleAPIStartTest);
  server.on("/api/reset_timer", HTTP_GET, handleAPIResetTimer);
  server.on("/api/load_questions", HTTP_POST, handleAPILoadQuestions);
  server.on("/api/load_questions", HTTP_OPTIONS, handleCORS);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("[SERVER] Web server started on port 80");
  Serial.println("[SERVER] Open http://" + WiFi.localIP().toString() + " in browser");
  Serial.println("\n============================================");
  Serial.println("   System Ready!");
  Serial.println("============================================\n");
}

// ===================== LOOP =====================

void loop() {
  // Handle web server
  server.handleClient();
  
  // Check keypad
  checkKeypad();
  
  // Check touch sensor
  checkTouch();
  
  // Update timer
  updateTimer();
  
  // Update buzzer
  buzzerUpdate();
  
  // Small delay to prevent WDT reset
  delay(1);
}