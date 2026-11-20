#pragma once
#include <stdint.h>

// Night mode (2026-11-20): between nightFrom and nightTo (local time, minutes
// after midnight, may wrap past midnight) only the faces in the nightFaces
// mask stay on, with the backlight turned right down; any other face goes
// dark and a tap shows it for NIGHT_PEEK_MS. The clock draws in dim red.
// Battery reminders are quiet during the same hours, even with night mode
// off. Settings in NVS (kubi_settings): nightOn, nightFrom, nightTo,
// nightFaces, nightLvl. Shared by the firmware and the simulator.

#define NIGHT_DEFAULT_FROM  (23 * 60)        // 23:00
#define NIGHT_DEFAULT_TO    (6 * 60 + 30)    // 06:30
#define NIGHT_DEFAULT_FACES (1 << 0)         // the clock (KubiMode bits: 0 clock,
                                             // 1 Pomodoro, 2 mascot, 3 ambient, 4 maze)
#define NIGHT_ALL_FACES     0x1F
#define NIGHT_DEFAULT_LEVEL 4                // 1 (faintest) .. NIGHT_LEVEL_COUNT
#define NIGHT_LEVEL_COUNT   10
#define NIGHT_PEEK_MS       10000UL          // a tap on a dark face shows it this long
#define NIGHT_PREVIEW_MS    30000UL          // the app's "Preview night" button

// True if minuteOfDay is inside [from, to), wrapping past midnight when
// from > to. from == to means no night at all.
inline bool nightWindowContains(int minuteOfDay, int from, int to) {
    if (from == to) return false;
    if (from < to) return minuteOfDay >= from && minuteOfDay < to;
    return minuteOfDay >= from || minuteOfDay < to;
}

inline bool nightFaceAllowed(int faces, int mode) {
    return mode >= 0 && mode < 5 && (faces & (1 << mode)) != 0;
}

// Backlight level (DisplayManager's 0-255 scale) for a night level. Levels
// below 16 use the extra-fine bottom end of the PWM (see dutyForLevel in
// DisplayManager.cpp): level 1 is 1/4095, about 0.02 % of full.
inline uint8_t nightBacklight(int level) {
    static const uint8_t LEVELS[NIGHT_LEVEL_COUNT] = { 1, 6, 7, 8, 9, 10, 11, 12, 14, 16 };
    if (level < 1) level = 1;
    if (level > NIGHT_LEVEL_COUNT) level = NIGHT_LEVEL_COUNT;
    return LEVELS[level - 1];
}
