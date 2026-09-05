#include "DisplayManager.h"
#include <cmath>

#define BACKLIGHT_PIN 32
#define PWM_CHANNEL   0
#define PWM_FREQ      5000
#define PWM_RES       8

DisplayManager display;

DisplayManager::DisplayManager()
    : _tft(),
      _currentBacklight(255),
      _targetBacklight(255),
      _sleeping(false),
      _wakeExpiryTime(0),
      _currentRotation(0) {}

void DisplayManager::init() {
    // 1. Initialize TFT
    _tft.init();
    _tft.setRotation(0); // Portrait 240x320 default
    _tft.invertDisplay(true); // Required for ST7789 IPS panels
    _tft.fillScreen(TFT_BLACK);

    // 2. Setup Backlight PWM on GPIO 32
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
    ledcAttachPin(BACKLIGHT_PIN, PWM_CHANNEL);
    setBacklight(255);
}

void DisplayManager::setBacklight(uint8_t brightness) {
    _currentBacklight = brightness;
    ledcWrite(PWM_CHANNEL, brightness);
}

void DisplayManager::setSleep(bool sleep) {
    _sleeping = sleep;
    if (sleep) {
        setBacklight(0);
    } else {
        setBacklight(255);
    }
}

void DisplayManager::wakeScreen(uint32_t durationMs) {
    _sleeping = false;
    setBacklight(255);
    _wakeExpiryTime = millis() + durationMs;
}

void DisplayManager::loop() {
    if (_wakeExpiryTime > 0 && millis() > _wakeExpiryTime) {
        _wakeExpiryTime = 0;
        setSleep(true);
    }
}

void DisplayManager::setRotation(uint8_t rotation) {
    if (_currentRotation != rotation) {
        _currentRotation = rotation;
        _tft.setRotation(rotation);
        _tft.fillScreen(TFT_BLACK); // Clean screen buffer on rotation
    }
}

void DisplayManager::setRotationForFace(int face) {
    // Physical Cube Orientation Mapping:
    // Face 1 Up: Standard Portrait (0)
    // Face 2 Up: Turned 90 deg sideways -> Landscape (1)
    // Face 3 Up: Inverted Portrait (2)
    // Face 4 Up: Turned 270 deg sideways -> Inverted Landscape (3)
    static const uint8_t faceToRotation[4] = { 0, 1, 2, 3 };
    if (face >= 0 && face < 4) {
        setRotation(faceToRotation[face]);
    }
}

void DisplayManager::drawBootScreen(const String& status) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    _tft.fillScreen(TFT_BLACK);
    
    // Logo
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("K U B I", cx, cy - 30, 4);

    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    _tft.drawString("Desk Companion", cx, cy + 5, 2);

    // Status message
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString(status, cx, h - 30, 2);
}

void DisplayManager::drawClockFace(int hour, int minute, int wday, int mday, int month, bool showDetails, bool isAnalog, const String& nextEvent, const String& ticker) {
    if (isAnalog) {
        drawAnalogClockFace(hour, minute, wday, mday, month);
        return;
    }

    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    _tft.fillScreen(TFT_BLACK);

    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hour, minute);

    char dateBuf[32] = "";
    if (wday >= 0 && wday < 7 && month >= 0 && month < 12 && mday >= 1 && mday <= 31) {
        static const char* const DAY_NAMES[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        static const char* const MONTH_NAMES[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        snprintf(dateBuf, sizeof(dateBuf), "%s, %d %s", DAY_NAMES[wday], mday, MONTH_NAMES[month]);
    }

    // Pure Minimalist Mode
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(timeBuf, cx, cy, 7); // Large 7-segment digital font

    if (dateBuf[0] != '\0') {
        _tft.drawString(dateBuf, cx, cy + 40, 2);
    }
}

void DisplayManager::drawAnalogClockFace(int hour, int minute, int wday, int mday, int month) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    _tft.fillScreen(TFT_BLACK);

    // 12 Numbers arranged in clock circle
    int radiusNumbers = 82;
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);

    for (int num = 1; num <= 12; num++) {
        float angleDeg = num * 30.0f - 90.0f;
        float angleRad = angleDeg * 0.0174532925f;
        int nx = cx + (int)round(radiusNumbers * cos(angleRad));
        int ny = cy + (int)round(radiusNumbers * sin(angleRad));

        char numStr[4];
        snprintf(numStr, sizeof(numStr), "%d", num);
        _tft.drawString(numStr, nx, ny, 2);
    }

    // Arm angles
    float hourVal = (hour % 12) + (minute / 60.0f);
    float hourAngleRad = (hourVal * 30.0f - 90.0f) * 0.0174532925f;
    float minAngleRad = (minute * 6.0f - 90.0f) * 0.0174532925f;

    // Hour arm: shorter and a nice orange colour (3px wide)
    int rHour = 44;
    int hx = cx + (int)round(rHour * cos(hourAngleRad));
    int hy = cy + (int)round(rHour * sin(hourAngleRad));

    float hdx = (float)(hx - cx);
    float hdy = (float)(hy - cy);
    float hlen = sqrt(hdx * hdx + hdy * hdy);
    if (hlen > 0.1f) {
        float hnx = -hdy / hlen;
        float hny = hdx / hlen;
        _tft.drawLine(cx, cy, hx, hy, TFT_ORANGE);
        _tft.drawLine(cx + (int)round(hnx), cy + (int)round(hny), hx + (int)round(hnx), hy + (int)round(hny), TFT_ORANGE);
        _tft.drawLine(cx - (int)round(hnx), cy - (int)round(hny), hx - (int)round(hnx), hy - (int)round(hny), TFT_ORANGE);
    }

    // Minute arm: long white line (2px wide)
    int rMin = 66;
    int mx = cx + (int)round(rMin * cos(minAngleRad));
    int my = cy + (int)round(rMin * sin(minAngleRad));

    float mdx = (float)(mx - cx);
    float mdy = (float)(my - cy);
    float mlen = sqrt(mdx * mdx + mdy * mdy);
    if (mlen > 0.1f) {
        float mnx = -mdy / mlen;
        float mny = mdx / mlen;
        _tft.drawLine(cx, cy, mx, my, TFT_WHITE);
        _tft.drawLine(cx + (int)round(mnx * 0.7f), cy + (int)round(mny * 0.7f),
                      mx + (int)round(mnx * 0.7f), my + (int)round(mny * 0.7f), TFT_WHITE);
    }

    // Center pivot
    _tft.fillCircle(cx, cy, 3, TFT_ORANGE);
    _tft.fillCircle(cx, cy, 1, TFT_WHITE);

    // Current date at bottom
    if (wday >= 0 && wday < 7 && month >= 0 && month < 12 && mday >= 1 && mday <= 31) {
        static const char* const DAY_NAMES[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        static const char* const MONTH_NAMES[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        char dateBuf[32];
        snprintf(dateBuf, sizeof(dateBuf), "%s, %d %s", DAY_NAMES[wday], mday, MONTH_NAMES[month]);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        _tft.drawString(dateBuf, cx, cy + 124, 2);
    }
}

void DisplayManager::drawPomodoroFace(int remainingSeconds, int totalSeconds, const char* phaseName, bool isPaused, bool isStarted, uint16_t textColor, int currentCycle, int cycleTarget) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    _tft.fillScreen(TFT_BLACK);

    int mins = remainingSeconds / 60;
    int secs = remainingSeconds % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);

    _tft.setTextDatum(MC_DATUM);

    // Phase Banner (top)
    _tft.setTextColor(textColor, TFT_BLACK);
    _tft.drawString(phaseName, cx, 28, 4);

    // Cycle Pips / Indicator (e.g. Cycle 2 of 4)
    if (cycleTarget > 0) {
        char cycleBuf[32];
        snprintf(cycleBuf, sizeof(cycleBuf), "Session %d of %d", currentCycle + 1, cycleTarget);
        _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        _tft.drawString(cycleBuf, cx, 52, 2);
    }

    // Huge Time Display in customized textColor!
    _tft.setTextColor(textColor, TFT_BLACK);
    _tft.drawString(buf, cx, cy + 5, 7);

    // Status Line
    if (!isStarted) {
        // Slow arcade-style flashing "START" (~1.2s cycle: 600ms on, 600ms off)
        if ((millis() % 1200) < 600) {
            _tft.setTextColor(TFT_WHITE, TFT_BLACK);
            _tft.drawString("START", cx, h - 38, 2);
        }
    } else if (isPaused) {
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);
        _tft.drawString("PAUSED", cx, h - 38, 2);
    }

    // Progress bar at bottom
    if (totalSeconds > 0) {
        int barMaxWidth = w - 40;
        int barWidth = (int)((1.0f - (float)remainingSeconds / (float)totalSeconds) * (float)barMaxWidth);
        if (barWidth > barMaxWidth) barWidth = barMaxWidth;
        if (barWidth < 0) barWidth = 0;
        _tft.drawRect(18, h - 18, barMaxWidth + 4, 8, TFT_DARKGREY);
        _tft.fillRect(20, h - 16, barWidth, 4, textColor);
    }
}

void DisplayManager::drawMascot(int centerX, int centerY, int routineState) {
    // Stylized Jelly Mascot (Kubi)
    int boxW = 100;
    int boxH = 85;
    int x = centerX - boxW / 2;
    int y = centerY - boxH / 2;

    uint16_t bodyColor = 0x2595; // Soft Jelly Cyan
    _tft.fillRoundRect(x, y, boxW, boxH, 16, bodyColor);
    _tft.drawRoundRect(x, y, boxW, boxH, 16, TFT_WHITE);

    // Eyes
    if (routineState == 0) {
        // Sleepy (Zzz)
        _tft.drawFastHLine(centerX - 25, centerY - 5, 16, TFT_WHITE);
        _tft.drawFastHLine(centerX + 10, centerY - 5, 16, TFT_WHITE);
        _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        _tft.drawString("z", centerX + 45, centerY - 45, 2);
        _tft.drawString("Z", centerX + 55, centerY - 60, 4);
    } else if (routineState == 1) {
        // Morning Coffee / Happy
        _tft.fillCircle(centerX - 18, centerY - 5, 5, TFT_WHITE);
        _tft.fillCircle(centerX + 18, centerY - 5, 5, TFT_WHITE);
        _tft.fillCircle(centerX - 17, centerY - 6, 2, TFT_BLACK);
        _tft.fillCircle(centerX + 19, centerY - 6, 2, TFT_BLACK);

        // Smile
        _tft.drawCircle(centerX, centerY + 12, 8, TFT_WHITE);
        _tft.fillRect(centerX - 9, centerY + 4, 18, 8, bodyColor);
    } else {
        // Focused / Reading
        _tft.fillCircle(centerX - 18, centerY - 5, 5, TFT_WHITE);
        _tft.fillCircle(centerX + 18, centerY - 5, 5, TFT_WHITE);
        _tft.drawFastHLine(centerX - 8, centerY + 12, 16, TFT_WHITE);
    }

    // Cheeks
    _tft.fillCircle(centerX - 32, centerY + 8, 4, 0xF9C0); // Pink cheeks
    _tft.fillCircle(centerX + 32, centerY + 8, 4, 0xF9C0);
}

void DisplayManager::drawMascotFace(float temperature, int hourOfDay) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    _tft.fillScreen(TFT_BLACK);

    // Determine routine
    int routine = 2; // Default focus
    if (hourOfDay >= 23 || hourOfDay < 6) {
        routine = 0; // Sleep
    } else if (hourOfDay >= 6 && hourOfDay < 10) {
        routine = 1; // Coffee morning
    }

    // Draw Mascot
    drawMascot(cx, cy - 25, routine);

    // Temperature Badge
    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f °C", temperature);

    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    _tft.drawString(tempBuf, cx, h - 45, 4);

    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("Room Temperature", cx, h - 20, 2);
}

void DisplayManager::drawScheduleFace(const std::vector<String>& events, int page) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;

    _tft.fillScreen(TFT_BLACK);

    _tft.setTextDatum(TC_DATUM);
    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    _tft.drawString("SCHEDULE (3 DAYS)", cx, 12, 2);
    _tft.drawFastHLine(20, 32, w - 40, TFT_DARKGREY);

    if (events.empty()) {
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        _tft.drawString("No upcoming events", cx, h / 2 - 10, 2);
        _tft.drawString("Sync via kubi.local", cx, h / 2 + 15, 2);
        return;
    }

    int maxItems = (h > 260) ? 4 : 3;
    int cardH = (h > 260) ? 46 : 38;
    int startIdx = page * maxItems;
    int y = 42;
    _tft.setTextDatum(TL_DATUM);

    for (size_t i = startIdx; i < events.size() && i < (size_t)(startIdx + maxItems); i++) {
        _tft.fillRoundRect(15, y, w - 30, cardH, 8, 0x18E3); // Dark card background
        _tft.setTextColor(TFT_WHITE, 0x18E3);
        _tft.drawString(events[i], 25, y + (cardH / 2) - 6, 2);
        y += cardH + 8;
    }

    _tft.setTextDatum(BC_DATUM);
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("Tap to cycle agenda", cx, h - 10, 1);
}

void DisplayManager::drawOverrideAlert(const String& message) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    // High-priority alert banner
    _tft.fillRoundRect(10, cy - 70, w - 20, 140, 12, TFT_MAROON);
    _tft.drawRoundRect(10, cy - 70, w - 20, 140, 12, TFT_RED);

    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_YELLOW, TFT_MAROON);
    _tft.drawString("! INCOMING ALERT !", cx, cy - 40, 2);

    _tft.setTextColor(TFT_WHITE, TFT_MAROON);
    _tft.drawString(message, cx, cy, 2);

    _tft.setTextColor(TFT_LIGHTGREY, TFT_MAROON);
    _tft.drawString("Tap to dismiss", cx, cy + 40, 1);
}
