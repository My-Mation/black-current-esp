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
#define TOUCH_THRESHOLD 1       // Digital mode: 1 = Touched, 0 = Idle
#define TOUCH_DEBOUNCE  200     // Faster response

// ---- 4x4 Keypad (HX543 layout) --------------------------
#define KP_ROWS 4
#define KP_COLS 4
// Row GPIO pins - Reversed to fix A->D, B->C mapping
#define KP_R1 32
#define KP_R2 33
#define KP_R3 25
#define KP_R4 26
// Column GPIO pins (Re-reversing to original working state)
#define KP_C1 27
#define KP_C2 14
#define KP_C3 12
#define KP_C4 13


// ---- System Timing ---------------------------------------
#define POLL_PERIOD_MS      1   // main loop small delay
#define TIMER_TICK_MS       1000
#define WIFI_TIMEOUT_TRIES  60  // x500ms = 30s
