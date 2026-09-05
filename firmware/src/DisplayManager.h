#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>

// Kubi Display Manager for 2.0" 240x320 IPS (ST7789 via SPI)
class DisplayManager {
public:
    DisplayManager();

    void init();
    void setBacklight(uint8_t brightness);
    void setSleep(bool sleep);
    bool isSleeping() const { return _sleeping; }
    void wakeScreen(uint32_t durationMs = 30000);
    void loop(); // Handles sleep timeout

    void drawBootScreen(const String& status);
    void drawClockFace(int hour, int minute, int wday, int mday, int month, bool showDetails, const String& nextEvent = "", const String& ticker = "");
    void drawClockFace(int hour, int minute, bool showDetails, const String& nextEvent = "", const String& ticker = "");
    void drawPomodoroFace(int remainingSeconds, int totalSeconds, const char* phaseName, bool isPaused, uint16_t textColor, int currentCycle = 0, int cycleTarget = 4);
    void drawMascotFace(float temperature, int hourOfDay);
    void drawScheduleFace(const std::vector<String>& events, int page = 0);
    void drawOverrideAlert(const String& message);

    void setRotation(uint8_t rotation);
    void setRotationForFace(int face);

private:
    TFT_eSPI _tft;
    uint8_t _currentBacklight;
    uint8_t _targetBacklight;
    bool _sleeping;
    uint32_t _wakeExpiryTime;
    uint8_t _currentRotation;

    void drawMascot(int centerX, int centerY, int state);
};

extern DisplayManager display;
