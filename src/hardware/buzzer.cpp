// =============================================================
//  buzzer.cpp — LEDC-based non-blocking buzzer (Core 2.x)
// =============================================================
#include "buzzer.h"

Buzzer gBuzzer;

void Buzzer::begin() {
    ledcSetup(BUZZER_LEDC_CH, BUZZER_FREQ, BUZZER_LEDC_RES);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CH);
    ledcWrite(BUZZER_LEDC_CH, 0);   
    Serial.println("[BUZZER] Core 2.x LEDC OK  pin=" + String(BUZZER_PIN));
}

void Buzzer::_startBeep(uint32_t freqHz, uint32_t durationMs) {
    ledcSetup(BUZZER_LEDC_CH, freqHz, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_LEDC_CH, 128);   // 50% duty
    _active = true;
    _endMs  = millis() + durationMs;
}

void Buzzer::beepKey() {
    _startBeep(2000, BUZZ_KEY_MS);
}

void Buzzer::beepConfirm() {
    _startBeep(2600, BUZZ_CONFIRM_MS);
}

void Buzzer::beepBoot() {
    ledcSetup(BUZZER_LEDC_CH, 1800, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_LEDC_CH, 128);
    delay(BUZZ_BOOT_MS);
    ledcWrite(BUZZER_LEDC_CH, 0);
    delay(90);
    ledcSetup(BUZZER_LEDC_CH, 2600, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_LEDC_CH, 128);
    delay(BUZZ_BOOT_MS);
    ledcWrite(BUZZER_LEDC_CH, 0);
    _active = false;
}

void Buzzer::update() {
    if (_active && millis() >= _endMs) {
        ledcWrite(BUZZER_LEDC_CH, 0);
        _active = false;
    }
}
