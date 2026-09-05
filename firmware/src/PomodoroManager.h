#pragma once
#include <Arduino.h>

enum PomodoroPhase {
    POMO_WORK = 0,
    POMO_SHORT_BREAK = 1,
    POMO_LONG_BREAK = 2
};

class PomodoroManager {
public:
    PomodoroManager();

    void init();
    void tick(); // Call once per second

    // Streamlined Gesture Handlers
    void handleTap();   // Dismiss chime if ringing, else toggle Pause/Play
    void handleShake(); // Skip to next phase

    // Direct actions (from web API or physical inputs)
    void play();
    void pause();
    void togglePause();
    void skipPhase();
    void resetCurrent();

    // Getters
    PomodoroPhase getPhase() const { return _phase; }
    const char* getPhaseName() const;
    int getRemainingSeconds() const { return _remainingSeconds; }
    int getTotalSeconds() const { return _totalSeconds; }
    bool isPaused() const { return _isPaused; }
    bool hasChimed() const { return _hasChimed; }
    int getCompletedCycles() const { return _completedCycles; }
    int getCycleTarget() const { return _cycleTarget; }

    // Durations
    int getFocusMinutes() const { return _focusMinutes; }
    int getShortBreakMinutes() const { return _shortBreakMinutes; }
    int getLongBreakMinutes() const { return _longBreakMinutes; }
    void setDurations(int focus, int shortBreak, int longBreak, int cycleTarget);

    // Color Configuration (RGB565 & Hex)
    uint16_t getPhaseColor() const;
    uint16_t getColorWork() const { return _colorWork; }
    uint16_t getColorShortBreak() const { return _colorShortBreak; }
    uint16_t getColorLongBreak() const { return _colorLongBreak; }
    void setColors(const String& hexWork, const String& hexShort, const String& hexLong);

    String getColorWorkHex() const;
    String getColorShortBreakHex() const;
    String getColorLongBreakHex() const;

    static uint16_t hexToRGB565(const String& hex);
    static String rgb565ToHex(uint16_t color);

private:
    PomodoroPhase _phase;
    int _remainingSeconds;
    int _totalSeconds;
    bool _isPaused;
    bool _hasChimed;
    int _completedCycles;

    // Configurable settings
    int _focusMinutes;
    int _shortBreakMinutes;
    int _longBreakMinutes;
    int _cycleTarget;

    // Color settings
    uint16_t _colorWork;
    uint16_t _colorShortBreak;
    uint16_t _colorLongBreak;

    void updatePhaseDurations();
    void advancePhase();
};

extern PomodoroManager pomodoro;
