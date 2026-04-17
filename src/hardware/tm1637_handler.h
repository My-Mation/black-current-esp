#pragma once
// =============================================================
//  tm1637_handler.h — 4-digit 7-segment timer display
//  Shows MM:SS elapsed for the current question
// =============================================================
#include <Arduino.h>
#include <TM1637Display.h>
#include "config.h"

class Tm1637Handler {
public:
    void begin();
    void showTime(unsigned int totalSeconds);
    void showDashes();   // Idle / not timing
    void showDone();     // "donE" when test finishes
    void update();       // Called from loop, syncs with gState
private:
    TM1637Display _dsp{TM_CLK, TM_DIO};
    unsigned int  _lastSec = 9999;  // force first draw
};

extern Tm1637Handler gTm;
