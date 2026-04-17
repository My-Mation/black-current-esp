#pragma once
// =============================================================
//  buzzer.h — Non-blocking buzzer feedback
// =============================================================
#include <Arduino.h>
#include "config.h"

class Buzzer {
public:
    void begin();
    void beepKey();        // Short blip for keypress
    void beepConfirm();    // Longer beep for confirmation/submit
    void beepBoot();       // Double beep at startup
    void update();         // Must be called from loop()

private:
    bool         _active = false;
    unsigned long _endMs = 0;
};

extern Buzzer gBuzzer;
