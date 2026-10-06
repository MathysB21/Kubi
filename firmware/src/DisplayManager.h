#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>

struct KubiScene;

// Kubi Display Manager for 2.0" 240x320 IPS (ST7789 via SPI)
class DisplayManager {
public:
    DisplayManager();

    void init();
    void setBacklight(uint8_t brightness);
    void setSleep(bool sleep);
    bool isSleeping() const { return _sleeping; }
    void loop(); // Inactivity timer + backlight fade. Call from the hardware loop.

    // --- Inactivity sleep ---
    // The orchestration loop reports activity (gesture, motion, face change)
    // and says whether the current face may sleep at all.
    bool noteActivity();              // Hardware loop only. Returns true if it woke the screen.
    void requestWake();               // Any task (e.g. API handlers): woken on next loop().
    void setSleepAllowed(bool allowed);
    void setSleepTimeoutMinutes(int minutes); // 0 = never sleep
    // Backlight level while awake (faces like Ambient run dimmer). Fades.
    void setAwakeBrightness(uint8_t level);
    int  getSleepTimeoutMinutes() const { return _sleepTimeoutMin; }

    void drawBootScreen(const String& status);
    void drawClockFace(int hour, int minute, int wday, int mday, int month, bool isAnalog);
    void drawAnalogClockFace(int hour, int minute, int wday = -1, int mday = 1, int month = 0);
    void drawPomodoroFace(int remainingSeconds, int totalSeconds, const char* phaseName, bool isPaused, bool isStarted, uint16_t textColor, int currentCycle = 0, int cycleTarget = 4);
    void drawMascotFace(float temperature, int hourOfDay);
    // Full-screen diorama scene with a temperature badge; falls back to the
    // vector mascot when no scenes are compiled in.
    void drawMascotFace(float temperature, int hourOfDay, int sceneIndex, bool showHud = true);
    void drawScene(int sceneIndex);
    void drawOverrideAlert(const String& message);
    void drawAmbientFace();
    void drawMazeFace();

    // --- Onboarding / maintenance screens ---
    void drawSetupScreen();                       // captive portal is up
    void drawAddressScreen(const String& ip);     // first connect: where the dashboard lives
    void drawResetScreen();                       // factory reset in progress

    void setRotation(uint8_t rotation);
    void setRotationForFace(int face);

    // Forces the next draw call to repaint its whole face from a cleared screen.
    void invalidate() { _fullRedraw = true; }

private:
    TFT_eSPI _tft;
    uint8_t _currentBacklight;
    uint8_t _targetBacklight;
    uint8_t _awakeBacklight;
    volatile bool _sleeping;
    volatile bool _wakeRequested;
    bool _sleepAllowed;
    int _sleepTimeoutMin;
    uint32_t _lastActivityTime;
    uint8_t _currentRotation;

    void drawMascot(int centerX, int centerY, int state);

    // --- Partial redraw bookkeeping ---
    // Steady-state frames only repaint what changed. A full clear happens on a
    // scene change, a rotation change or invalidate(): a 240x320 fillScreen
    // costs ~31 ms of SPI at 40 MHz, 61% of a 50 ms frame.
    enum Scene : uint8_t {
        SCENE_NONE = 0,
        SCENE_BOOT,
        SCENE_CLOCK_DIGITAL,
        SCENE_CLOCK_ANALOG,
        SCENE_POMODORO,
        SCENE_MASCOT,
        SCENE_MASCOT_DIORAMA,
        SCENE_OVERRIDE,
        SCENE_AMBIENT,
        SCENE_MAZE,
        SCENE_ADDRESS
    };
    Scene _scene;
    bool _fullRedraw;
    // Returns true if the face must be painted from scratch. Clears the screen
    // first unless clear is false (overlays such as the override banner).
    bool beginScene(Scene scene, bool clear = true);

    // Last-drawn values; only meaningful while _scene is unchanged
    int _drawnHour, _drawnMinute, _drawnMday;
    int _drawnRemaining, _drawnBarWidth, _drawnCycle;
    uint8_t _drawnStatus;
    char _drawnPhase[24];
    uint16_t _drawnColor;
    int _drawnRoutine;
    const KubiScene* _drawnDiorama;
    char _drawnTemp[16];
    uint32_t _drawnOverrideSig;
    uint32_t _drawnAddressSig;
};

extern DisplayManager display;
