#ifndef CONFIG_H
#define CONFIG_H

// ============== WiFi Configuration ==============
#define WIFI_SSID "OPPO K13x 5G s3cc"
#define WIFI_PASS ""  // Open network, no password

// ============== Pin Definitions ==============

// OLED Display (I2C)
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

// TM1637 7-Segment Display
#define TM1637_CLK 18
#define TM1637_DIO 19

// Buzzer
#define BUZZER_PIN 5

// Touch Sensor
#define TOUCH_PIN 4
#define TOUCH_THRESHOLD 40  // Capacitive touch threshold

// 4x4 Keypad (HX543)
#define KP_ROW1 13
#define KP_ROW2 12
#define KP_ROW3 14
#define KP_ROW4 27
#define KP_COL1 26
#define KP_COL2 25
#define KP_COL3 33
#define KP_COL4 32

// ============== Timing ==============
#define DEBOUNCE_MS 200
#define TIMER_UPDATE_MS 1000
#define TOUCH_DEBOUNCE_MS 500
#define BUZZER_BEEP_MS 50
#define BUZZER_SUBMIT_MS 200
#define BUZZER_FREQ 2000

// ============== Web Server ==============
#define WEB_PORT 80

#endif // CONFIG_H
