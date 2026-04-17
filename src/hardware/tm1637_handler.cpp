// =============================================================
//  tm1637_handler.cpp — TM1637 7-segment implementation
// =============================================================
#include "tm1637_handler.h"
#include "../test_engine/test_state.h"

Tm1637Handler gTm;

void Tm1637Handler::begin() {
    _dsp.setBrightness(5);
    showDashes();
    Serial.println("[TM1637] OK");
}

void Tm1637Handler::showDashes() {
    uint8_t dash[] = {SEG_G, SEG_G, SEG_G, SEG_G};
    _dsp.setSegments(dash);
    _lastSec = 9999;
}

void Tm1637Handler::showTime(unsigned int totalSeconds) {
    if (totalSeconds == _lastSec) return;   // nothing changed
    _lastSec = totalSeconds;

    int m = (totalSeconds / 60) % 100;
    int s = totalSeconds % 60;

    uint8_t segs[4];
    segs[0] = _dsp.encodeDigit(m / 10);
    segs[1] = _dsp.encodeDigit(m % 10) | 0x80;  // 0x80 = colon bit
    segs[2] = _dsp.encodeDigit(s / 10);
    segs[3] = _dsp.encodeDigit(s % 10);
    _dsp.setSegments(segs);
}

void Tm1637Handler::showDone() {
    // "donE" on 7-segment
    uint8_t data[] = {
        SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,   // d
        SEG_C | SEG_D | SEG_E | SEG_G,            // o
        SEG_C | SEG_E | SEG_G,                    // n
        SEG_A | SEG_D | SEG_E | SEG_F | SEG_G     // E
    };
    _dsp.setSegments(data);
    _lastSec = 9999;
}

void Tm1637Handler::update() {
    if (gState.isDone()) {
        showDone();
    } else if (gState.timerRunning) {
        showTime(gState.elapsedSec);
    } else {
        showDashes();
    }
}
