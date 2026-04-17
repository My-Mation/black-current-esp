#pragma once
// =============================================================
//  config.h — Hardware pin definitions and system constants
//  All hardware-specific values live here for easy modification
// =============================================================

// ---- WiFi ------------------------------------------------
#define WIFI_SSID   "OPPO K13x 5G s3cc"
#define WIFI_PASS   ""          // Open network

// ---- Web Server ------------------------------------------
#define WEB_PORT    80

// ---- OLED (I2C SSD1306) ----------------------------------
#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C

// ---- TM1637 7-Segment ------------------------------------
#define TM_CLK      18
#define TM_DIO      19

// ---- Buzzer ----------------------------------------------
#define BUZZER_PIN      5
#define BUZZER_FREQ     2000    // Hz
#define BUZZ_KEY_MS     50      // keypress beep duration
#define BUZZ_CONFIRM_MS 200     // confirm/submit beep duration
#define BUZZ_BOOT_MS    100     // boot sequence beep

// ---- Touch Sensor ----------------------------------------
#define TOUCH_PIN       4
#define TOUCH_THRESHOLD 40      // Below this = touched (capacitive)
#define TOUCH_DEBOUNCE  600     // ms between valid touches

// ---- 4x4 Keypad (HX543 layout) --------------------------
#define KP_ROWS 4
#define KP_COLS 4
// Row GPIO pins (top to bottom)
#define KP_R1 13
#define KP_R2 12
#define KP_R3 14
#define KP_R4 27
// Column GPIO pins (left to right)
#define KP_C1 26
#define KP_C2 25
#define KP_C3 33
#define KP_C4 32

// ---- System Timing ---------------------------------------
#define POLL_PERIOD_MS      1   // main loop small delay
#define TIMER_TICK_MS       1000
#define WIFI_TIMEOUT_TRIES  60  // x500ms = 30s
