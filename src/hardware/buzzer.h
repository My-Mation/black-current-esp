#pragma once
// =============================================================
//  buzzer.h — Non-blocking buzzer feedback via LEDC PWM
//  ESP32's tone() is unreliable on newer arduino-esp32 versions;
//  using LEDC directly guarantees the pin is actually driven.
// =============================================================
#include <Arduino.h>
#include "config.h"

#define BUZZER_LEDC_CH   0    // LEDC channel (0–15)
#define BUZZER_LEDC_RES  8    // 8-bit resolution (0–255 duty)

class Buzzer {
public:
    void begin();
    void beepKey();        // ~60 ms short blip — every keypress
    void beepConfirm();    // ~200 ms firm beep — confirmation/submit
    void beepBoot();       // Blocking double chirp at startup (only in setup())
    void update();         // Must be called every loop() iteration

private:
    bool          _active = false;
    unsigned long _endMs  = 0;

    // Internal helper — starts a non-blocking LEDC tone
    void _startBeep(uint32_t freqHz, uint32_t durationMs);
};

extern Buzzer gBuzzer;
