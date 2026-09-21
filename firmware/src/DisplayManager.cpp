#include "DisplayManager.h"
#include "AmbientFace.h"
#include "MazeGame.h"
#include "FaceMap.h"
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
      _awakeBacklight(255),
      _sleeping(false),
      _wakeRequested(false),
      _sleepAllowed(false),
      _sleepTimeoutMin(5),
      _lastActivityTime(0),
      _scene(SCENE_NONE),
      _fullRedraw(true),
      _drawnHour(-1), _drawnMinute(-1), _drawnMday(-1),
      _drawnRemaining(-1), _drawnBarWidth(0), _drawnCycle(-1),
      _drawnStatus(0xFF),
      _drawnColor(0),
      _drawnRoutine(-1),
      _drawnOverrideSig(0),
      _drawnAddressSig(0),
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

// Backlight steps per loop() call while fading (~10 ms per call -> ~0.35 s full range)
#define BACKLIGHT_FADE_STEP 8

void DisplayManager::setSleep(bool sleep) {
    _sleeping = sleep;
    _targetBacklight = sleep ? 0 : _awakeBacklight;
    if (!sleep) {
        setBacklight(_awakeBacklight); // Wake is instant; only the fade-out is gradual
    }
}

void DisplayManager::setAwakeBrightness(uint8_t level) {
    if (level == _awakeBacklight) return;
    _awakeBacklight = level;
    if (!_sleeping) _targetBacklight = level; // loop() fades towards it
}

bool DisplayManager::noteActivity() {
    _lastActivityTime = millis();
    if (_sleeping) {
        setSleep(false);
        Serial.println("[DISPLAY] Woke from inactivity sleep");
        return true;
    }
    return false;
}

void DisplayManager::requestWake() {
    _wakeRequested = true;
}

void DisplayManager::setSleepAllowed(bool allowed) {
    if (allowed == _sleepAllowed) return;
    _sleepAllowed = allowed;
    _lastActivityTime = millis(); // Timer starts fresh when entering a sleeping face
    if (!allowed && _sleeping) {
        setSleep(false);
    }
}

void DisplayManager::setSleepTimeoutMinutes(int minutes) {
    if (minutes < 0) minutes = 0;
    if (minutes > 120) minutes = 120;
    _sleepTimeoutMin = minutes;
    _lastActivityTime = millis();
    if (minutes == 0 && _sleeping) {
        setSleep(false);
    }
}

void DisplayManager::loop() {
    if (_wakeRequested) {
        _wakeRequested = false;
        noteActivity();
    }

    if (!_sleeping && _sleepAllowed && _sleepTimeoutMin > 0 &&
        millis() - _lastActivityTime > (uint32_t)_sleepTimeoutMin * 60000UL) {
        Serial.println("[DISPLAY] Inactivity timeout -> sleeping");
        setSleep(true);
    }

    // Gentle fade towards the target backlight level, either direction
    if (_currentBacklight > _targetBacklight) {
        int next = (int)_currentBacklight - BACKLIGHT_FADE_STEP;
        setBacklight(next < (int)_targetBacklight ? _targetBacklight : (uint8_t)next);
    } else if (_currentBacklight < _targetBacklight) {
        int next = (int)_currentBacklight + BACKLIGHT_FADE_STEP;
        setBacklight(next > (int)_targetBacklight ? _targetBacklight : (uint8_t)next);
    }
}

void DisplayManager::setRotation(uint8_t rotation) {
    if (_currentRotation != rotation) {
        _currentRotation = rotation;
        _tft.setRotation(rotation);
        _tft.fillScreen(TFT_BLACK); // Clean screen buffer on rotation
        _fullRedraw = true;
    }
}

void DisplayManager::setRotationForFace(int face) {
    // Rotation per face comes from the shared table in FaceMap.h
    if (face >= 0 && face < FACE_COUNT) {
        setRotation(FACE_POSES[face].rotation);
    }
}

// FNV-1a, for cheap "did this content change" checks
static uint32_t fnv1a(uint32_t h, const char* str) {
    if (h == 0) h = 2166136261u;
    while (*str) { h ^= (uint8_t)*str++; h *= 16777619u; }
    return h ^ 0xFF; // separator so "ab"+"c" != "a"+"bc"
}

static const char* const DAY_NAMES[]   = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char* const MONTH_NAMES[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static bool formatDate(char* buf, size_t len, int wday, int mday, int month) {
    if (wday >= 0 && wday < 7 && month >= 0 && month < 12 && mday >= 1 && mday <= 31) {
        snprintf(buf, len, "%s, %d %s", DAY_NAMES[wday], mday, MONTH_NAMES[month]);
        return true;
    }
    buf[0] = '\0';
    return false;
}

bool DisplayManager::beginScene(Scene scene, bool clear) {
    if (scene == _scene && !_fullRedraw) return false;
    _scene = scene;
    _fullRedraw = false;
    if (clear) _tft.fillScreen(TFT_BLACK);
    return true;
}

void DisplayManager::drawBootScreen(const String& status) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    // Boot is not steady state: repaint it whole, then force the first face to.
    _scene = SCENE_BOOT;
    _fullRedraw = true;
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

void DisplayManager::drawClockFace(int hour, int minute, int wday, int mday, int month, bool isAnalog) {
    if (isAnalog) {
        drawAnalogClockFace(hour, minute, wday, mday, month);
        return;
    }

    bool full = beginScene(SCENE_CLOCK_DIGITAL);
    // The clock only changes once a minute: skip the frame entirely otherwise
    if (!full && hour == _drawnHour && minute == _drawnMinute && mday == _drawnMday) return;

    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hour, minute);

    char dateBuf[32];
    bool hasDate = formatDate(dateBuf, sizeof(dateBuf), wday, mday, month);

    // Pure Minimalist Mode. Opaque text + full-width padding overwrites the
    // previous minute in place, no clear needed.
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextPadding(w);
    _tft.drawString(timeBuf, cx, cy, 7); // Large 7-segment digital font

    if (hasDate && (full || mday != _drawnMday)) {
        _tft.drawString(dateBuf, cx, cy + 40, 2);
    }
    _tft.setTextPadding(0);

    _drawnHour = hour;
    _drawnMinute = minute;
    _drawnMday = mday;
}

void DisplayManager::drawAnalogClockFace(int hour, int minute, int wday, int mday, int month) {
    bool full = beginScene(SCENE_CLOCK_ANALOG);
    if (!full && hour == _drawnHour && minute == _drawnMinute && mday == _drawnMday) return;

    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);

    if (full) {
        // 12 Numbers arranged in clock circle (static, drawn once)
        int radiusNumbers = 82;
        for (int num = 1; num <= 12; num++) {
            float angleDeg = num * 30.0f - 90.0f;
            float angleRad = angleDeg * 0.0174532925f;
            int nx = cx + (int)round(radiusNumbers * cos(angleRad));
            int ny = cy + (int)round(radiusNumbers * sin(angleRad));

            char numStr[4];
            snprintf(numStr, sizeof(numStr), "%d", num);
            _tft.drawString(numStr, nx, ny, 2);
        }
    } else {
        // Erase the old hands only. Radius 70 stays inside the numerals (82 - ~8).
        _tft.fillCircle(cx, cy, 70, TFT_BLACK);
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
    char dateBuf[32];
    if ((full || mday != _drawnMday) && formatDate(dateBuf, sizeof(dateBuf), wday, mday, month)) {
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        _tft.setTextPadding(w);
        _tft.drawString(dateBuf, cx, cy + 124, 2);
        _tft.setTextPadding(0);
    }

    _drawnHour = hour;
    _drawnMinute = minute;
    _drawnMday = mday;
}

void DisplayManager::drawPomodoroFace(int remainingSeconds, int totalSeconds, const char* phaseName, bool isPaused, bool isStarted, uint16_t textColor, int currentCycle, int cycleTarget) {
    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;
    int cy = h / 2;

    // A colour change recolours everything: treat it as a new scene
    if (_scene == SCENE_POMODORO && textColor != _drawnColor) invalidate();
    bool full = beginScene(SCENE_POMODORO);

    _tft.setTextDatum(MC_DATUM);
    _tft.setTextPadding(w); // every text row below overwrites its own band

    // Phase Banner (top)
    if (full || strncmp(phaseName, _drawnPhase, sizeof(_drawnPhase)) != 0) {
        _tft.setTextColor(textColor, TFT_BLACK);
        _tft.drawString(phaseName, cx, 28, 4);
        strncpy(_drawnPhase, phaseName, sizeof(_drawnPhase) - 1);
        _drawnPhase[sizeof(_drawnPhase) - 1] = '\0';
    }

    // Cycle Pips / Indicator (e.g. Cycle 2 of 4)
    if (cycleTarget > 0 && (full || currentCycle != _drawnCycle)) {
        char cycleBuf[32];
        snprintf(cycleBuf, sizeof(cycleBuf), "Session %d of %d", currentCycle + 1, cycleTarget);
        _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        _tft.drawString(cycleBuf, cx, 52, 2);
        _drawnCycle = currentCycle;
    }

    // Huge Time Display in customized textColor! Changes once a second.
    if (full || remainingSeconds != _drawnRemaining) {
        int mins = remainingSeconds / 60;
        int secs = remainingSeconds % 60;
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
        _tft.setTextColor(textColor, TFT_BLACK);
        _tft.drawString(buf, cx, cy + 5, 7);
        _drawnRemaining = remainingSeconds;
    }

    // Status Line: 0 = none, 1 = START shown, 2 = START blinked off, 3 = PAUSED
    uint8_t status = 0;
    if (!isStarted) {
        // Slow arcade-style flashing "START" (~1.2s cycle: 600ms on, 600ms off)
        status = ((millis() % 1200) < 600) ? 1 : 2;
    } else if (isPaused) {
        status = 3;
    }
    if (full || status != _drawnStatus) {
        if (status == 1 || status == 3) {
            _tft.setTextColor(TFT_WHITE, TFT_BLACK);
            _tft.drawString(status == 1 ? "START" : "PAUSED", cx, h - 38, 2);
        } else {
            _tft.fillRect(0, h - 38 - 9, w, 18, TFT_BLACK);
        }
        _drawnStatus = status;
    }
    _tft.setTextPadding(0);

    // Progress bar at bottom: only the changed span of the fill is touched
    if (totalSeconds > 0) {
        int barMaxWidth = w - 40;
        int barWidth = (int)((1.0f - (float)remainingSeconds / (float)totalSeconds) * (float)barMaxWidth);
        if (barWidth > barMaxWidth) barWidth = barMaxWidth;
        if (barWidth < 0) barWidth = 0;
        if (full) {
            _tft.drawRect(18, h - 18, barMaxWidth + 4, 8, TFT_DARKGREY);
            _drawnBarWidth = 0;
        }
        if (barWidth > _drawnBarWidth) {
            _tft.fillRect(20 + _drawnBarWidth, h - 16, barWidth - _drawnBarWidth, 4, textColor);
        } else if (barWidth < _drawnBarWidth) {
            _tft.fillRect(20 + barWidth, h - 16, _drawnBarWidth - barWidth, 4, TFT_BLACK);
        }
        _drawnBarWidth = barWidth;
    }

    _drawnColor = textColor;
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
        // Sleepy (Zzz). Transparent text (fg == bg) so the z's don't punch
        // black boxes into the body corner they overlap.
        _tft.drawFastHLine(centerX - 25, centerY - 5, 16, TFT_WHITE);
        _tft.drawFastHLine(centerX + 10, centerY - 5, 16, TFT_WHITE);
        _tft.setTextColor(TFT_YELLOW, TFT_YELLOW);
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

    // Determine routine
    int routine = 2; // Default focus
    if (hourOfDay >= 23 || hourOfDay < 6) {
        routine = 0; // Sleep
    } else if (hourOfDay >= 6 && hourOfDay < 10) {
        routine = 1; // Coffee morning
    }

    // The mascot is static between routine changes: repaint it only then
    if (_scene == SCENE_MASCOT && routine != _drawnRoutine) invalidate();
    bool full = beginScene(SCENE_MASCOT);

    if (full) {
        drawMascot(cx, cy - 25, routine);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        _tft.drawString("Room Temperature", cx, h - 20, 2);
        _drawnRoutine = routine;
    }

    // Temperature Badge
    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f °C", temperature);
    if (full || strcmp(tempBuf, _drawnTemp) != 0) {
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(TFT_GOLD, TFT_BLACK);
        _tft.setTextPadding(w);
        _tft.drawString(tempBuf, cx, h - 45, 4);
        _tft.setTextPadding(0);
        strncpy(_drawnTemp, tempBuf, sizeof(_drawnTemp) - 1);
        _drawnTemp[sizeof(_drawnTemp) - 1] = '\0';
    }
}

// Shown while the Kubi-Setup hotspot is up. Not steady state: full repaint.
void DisplayManager::drawSetupScreen() {
    int cx = _tft.width() / 2;
    _scene = SCENE_BOOT;
    _fullRedraw = true;
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);

    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("Hello!", cx, 40, 4);

    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("On your phone,", cx, 90, 2);
    _tft.drawString("join this WiFi:", cx, 110, 2);
    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    _tft.drawString("Kubi-Setup", cx, 142, 4);

    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("A page will open.", cx, 192, 2);
    _tft.drawString("If not, go to", cx, 212, 2);
    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    _tft.drawString("192.168.4.1", cx, 236, 2);

    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("then pick your WiFi", cx, 280, 2);
}

// First boot after setup: tell the owner where the dashboard is. Stays up until
// tapped (the caller owns the timeout); only repaints if the address changes.
void DisplayManager::drawAddressScreen(const String& ip) {
    uint32_t sig = fnv1a(0, ip.c_str());
    if (_scene == SCENE_ADDRESS && sig != _drawnAddressSig) invalidate();
    if (!beginScene(SCENE_ADDRESS)) return;
    _drawnAddressSig = sig;

    int cx = _tft.width() / 2;
    int cy = _tft.height() / 2;
    _tft.setTextDatum(MC_DATUM);

    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    _tft.drawString("You're connected!", cx, cy - 100, 2);

    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("On your phone, open", cx, cy - 60, 2);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("kubi.local", cx, cy - 28, 4);

    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("or", cx, cy + 4, 2);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(ip, cx, cy + 36, 4);

    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("Tap Kubi when done", cx, cy + 100, 2);
}

void DisplayManager::drawResetScreen() {
    int cx = _tft.width() / 2;
    int cy = _tft.height() / 2;
    _scene = SCENE_BOOT;
    _fullRedraw = true;
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);

    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("Factory reset", cx, cy - 40, 4);
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("Erasing WiFi and", cx, cy, 2);
    _tft.drawString("all settings...", cx, cy + 20, 2);
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("Kubi will restart", cx, cy + 56, 2);
    _tft.drawString("into setup mode.", cx, cy + 76, 2);
}

void DisplayManager::drawMazeFace() {
    bool full = beginScene(SCENE_MAZE);
    maze.draw(_tft, full, millis());
}

void DisplayManager::drawAmbientFace() {
    bool full = beginScene(SCENE_AMBIENT);
    ambient.draw(_tft, full, millis());
}

void DisplayManager::drawOverrideAlert(const String& message) {
    // Overlay on top of the current face (no clear); repaint only when the
    // message changes. Dismissing it changes scene, which repaints the face.
    uint32_t sig = fnv1a(0, message.c_str());
    if (_scene == SCENE_OVERRIDE && sig != _drawnOverrideSig) invalidate();
    if (!beginScene(SCENE_OVERRIDE, false)) return;
    _drawnOverrideSig = sig;

    int w = _tft.width();
    int cx = w / 2;
    int cy = _tft.height() / 2;

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
