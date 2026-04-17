#pragma once
// =============================================================
//  touch.h — Multi-stage touch sensor logic
//
//  Touch behavior per mode:
//    MCQ:   Touch → confirm selected option → move to next Q
//    NUM:   Touch → confirm numeric buffer   → move to next Q
//    VOICE: Touch 1 → REC_START (browser starts recording)
//           Touch 2 → REC_STOP  (browser stops recording)
//           Touch 3 → CONFIRMED (move to next Q)
//
//  Each touch fires a TouchStage event into gState for the
//  browser poll (/api/state) to consume.
// =============================================================
#include <Arduino.h>
#include "config.h"

class TouchHandler {
public:
    void begin();
    void update();   // Poll-based capacitive touch check (from loop)

private:
    unsigned long _lastTouchMs = 0;
    bool          _prevTouched = false;

    void _handleTouch();
};

extern TouchHandler gTouch;
