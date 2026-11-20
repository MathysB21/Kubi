#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Face 4 (default): a slow pixel-art glow for a dark room.
// A dithered orb of 8 px cells breathes and drifts through a night palette.
// Only cells whose colour changed are pushed, so it costs little SPI.
class AmbientFace {
public:
    static const uint8_t  BACKLIGHT = 48;     // dim: this face lives in dark rooms
    static const uint32_t STOP_MS   = 40000;  // time spent easing to each palette colour

    // full = the screen was just cleared. Throttles itself to ~10 Hz.
    void draw(TFT_eSPI& tft, bool full, uint32_t nowMs);
    void nextColour() { _phaseOffsetMs += STOP_MS; } // tap: move on to the next colour

private:
    static const int CELL = 8;
    uint16_t _cells[(240 / CELL) * (320 / CELL)];
    uint32_t _phaseOffsetMs = 0;
    uint32_t _lastDrawMs = 0;
};

extern AmbientFace ambient;
