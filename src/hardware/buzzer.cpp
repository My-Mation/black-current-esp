// =============================================================
//  buzzer.cpp — Non-blocking buzzer implementation
// =============================================================
#include "buzzer.h"

Buzzer gBuzzer;

void Buzzer::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void Buzzer::beepKey() {
    tone(BUZZER_PIN, BUZZER_FREQ, BUZZ_KEY_MS);
    _active = true;
    _endMs  = millis() + BUZZ_KEY_MS;
}

void Buzzer::beepConfirm() {
    tone(BUZZER_PIN, BUZZER_FREQ, BUZZ_CONFIRM_MS);
    _active = true;
    _endMs  = millis() + BUZZ_CONFIRM_MS;
}

void Buzzer::beepBoot() {
    // Two ascending chirps
    tone(BUZZER_PIN, 1800, BUZZ_BOOT_MS);
    delay(120);
    tone(BUZZER_PIN, 2400, BUZZ_BOOT_MS);
    _active = true;
    _endMs  = millis() + BUZZ_BOOT_MS;
}

void Buzzer::update() {
    if (_active && millis() >= _endMs) {
        noTone(BUZZER_PIN);
        _active = false;
    }
}
