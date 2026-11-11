#include "DisplayManager.h"
#include "AmbientFace.h"
#include "MazeGame.h"
#include "FaceMap.h"
#include "KubiScenes.h"
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
      _drawnDiorama(nullptr),
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

void DisplayManager::powerDown() {
    setBacklight(0);
    _tft.writecommand(0x28); // DISPOFF
    delay(10);
    _tft.writecommand(0x10); // SLPIN; the next init() resets the panel anyway
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

static const KubiScene* sceneAt(int sceneIndex) {
    if (ALL_KUBI_SCENE_COUNT == 0) return nullptr;
    int idx = sceneIndex % (int)ALL_KUBI_SCENE_COUNT;
    if (idx < 0) idx += (int)ALL_KUBI_SCENE_COUNT;
    const KubiScene* sc = ALL_KUBI_SCENES[idx];
    return (sc && sc->palette && sc->rleData) ? sc : nullptr;
}

// --- Diorama animation (tools/build_scenes.py SCENE_ANIM) ---
// The drawn scene is kept decoded (one palette index per scene pixel, 19 KB)
// so anything drawn over it can be erased by repainting just those pixels.
static const int ANIM_W = 120, ANIM_H = 160;
static uint8_t s_sceneIdx[ANIM_W * ANIM_H];
static const KubiScene* s_idxScene = nullptr;

struct SceneParticle {
    bool alive, drawn;
    int16_t x16, y16;   // 1/16 scene pixel
    int8_t vx16, vy16;
    int8_t px, py;      // where it was drawn
    uint8_t age;
    uint16_t color;
};
static const int MAX_SCENE_PARTICLES = 8;
static SceneParticle s_particles[MAX_SCENE_PARTICLES];
static uint32_t s_nextTick, s_nextBlink, s_blinkUntil, s_nextSpark;
static uint8_t s_spawnCountdown;
static bool s_blinkShut;

struct SceneSpark { bool on; uint8_t x, y, stage; uint32_t until; };
static SceneSpark s_sparks[3];
static int s_rippleRing;           // ring lit now, -1 while resting
static uint32_t s_nextRipple, s_nextWiggle;
static uint8_t s_wigglePhase;      // 0 still, 1 shifted right, 2 still, 3 shifted left

static uint32_t s_rng = 0x2545F491u;
static uint32_t animRand() { s_rng ^= s_rng << 13; s_rng ^= s_rng >> 17; s_rng ^= s_rng << 5; return s_rng; }
static int animRange(int n) { return n > 0 ? (int)(animRand() % (uint32_t)n) : 0; }

// The temperature badge, in scene pixels: never drawn on or restored
static bool inHud(int x, int y) { return x >= 34 && x <= 86 && y >= 141 && y <= 157; }

static void resetSceneAnimation(uint32_t now) {
    for (auto& p : s_particles) p.alive = p.drawn = false;
    for (auto& s : s_sparks) s.on = false;
    s_blinkShut = false;
    s_nextTick = now;
    s_nextBlink = now + 2000 + animRange(3000);
    s_nextSpark = now + 500;
    s_spawnCountdown = 0;
    s_rippleRing = -1;
    s_nextRipple = now + 300;
    s_wigglePhase = 0;
    s_nextWiggle = now;
}

static void paintScenePixel(TFT_eSPI& tft, int x, int y, uint8_t scale, uint16_t color);
static void restoreScenePixel(TFT_eSPI& tft, const KubiScene* sc, int x, int y);

static void drawAnimPixels(TFT_eSPI& tft, const KubiScene* sc, const KubiAnimPixel* px, int count, bool restore) {
    for (int i = 0; i < count; i++) {
        if (restore) {
            restoreScenePixel(tft, sc, px[i].x, px[i].y);
        } else {
            uint8_t c = px[i].c < sc->paletteSize ? px[i].c : 0;
            paintScenePixel(tft, px[i].x, px[i].y, sc->scale, pgm_read_word(&sc->palette[c]));
        }
    }
}

static void drawRippleRing(TFT_eSPI& tft, const KubiScene* sc, int ring, bool lit) {
    const KubiSceneAnim* a = sc->anim;
    for (int i = 0; i < a->rippleCount; i++) {
        if (a->ripple[i].c != ring) continue;
        if (lit) paintScenePixel(tft, a->ripple[i].x, a->ripple[i].y, sc->scale, 0xFFFF);
        else restoreScenePixel(tft, sc, a->ripple[i].x, a->ripple[i].y);
    }
}

static void paintScenePixel(TFT_eSPI& tft, int x, int y, uint8_t scale, uint16_t color) {
    if (x < 0 || y < 0 || x >= ANIM_W || y >= ANIM_H || inHud(x, y)) return;
    tft.fillRect(x * scale, y * scale, scale, scale, color);
}

static void restoreScenePixel(TFT_eSPI& tft, const KubiScene* sc, int x, int y) {
    if (x < 0 || y < 0 || x >= ANIM_W || y >= ANIM_H) return;
    uint8_t idx = s_sceneIdx[y * ANIM_W + x];
    if (idx >= sc->paletteSize) idx = 0;
    paintScenePixel(tft, x, y, sc->scale, pgm_read_word(&sc->palette[idx]));
}

static void glyphPixels(TFT_eSPI& tft, const KubiScene* sc, int px, int py, bool erase, uint16_t color) {
    const KubiSceneAnim* a = sc->anim;
    for (int gy = 0; gy < a->glyphH; gy++) {
        for (int gx = 0; gx < a->glyphW; gx++) {
            if (!(a->glyphMask & (1u << (gy * 4 + gx)))) continue;
            if (erase) restoreScenePixel(tft, sc, px + gx, py + gy);
            else paintScenePixel(tft, px + gx, py + gy, sc->scale, color);
        }
    }
}

void DisplayManager::pokeScene() {
    if (!s_blinkShut) s_nextBlink = millis();
}

void DisplayManager::animateScene(const KubiScene* sc, uint32_t now) {
    if (!sc || sc != s_idxScene || !sc->anim || _sleeping) return;
    const KubiSceneAnim* a = sc->anim;
    uint8_t scale = sc->scale;

    // Blink: eyes shut for ~140 ms every few seconds, sometimes twice
    if (a->eyeCount && a->eyes) {
        if (!s_blinkShut && (int32_t)(now - s_nextBlink) >= 0) {
            for (int i = 0; i < a->eyeCount; i++) paintScenePixel(_tft, a->eyes[i].x, a->eyes[i].y, scale, a->lidColor);
            s_blinkShut = true;
            s_blinkUntil = now + 140;
        } else if (s_blinkShut && (int32_t)(now - s_blinkUntil) >= 0) {
            for (int i = 0; i < a->eyeCount; i++) restoreScenePixel(_tft, sc, a->eyes[i].x, a->eyes[i].y);
            s_blinkShut = false;
            s_nextBlink = now + (animRange(5) == 0 ? 220 : 2500 + animRange(4000));
        }
    }

    // Glints: a small plus that shrinks to a dot, then the scene again
    if (a->sparkCount && a->sparks) {
        if ((int32_t)(now - s_nextSpark) >= 0) {
            for (auto& s : s_sparks) {
                if (s.on) continue;
                const KubiAnimPoint& p = a->sparks[animRange(a->sparkCount)];
                s = { true, p.x, p.y, 0, now + 120 };
                paintScenePixel(_tft, p.x, p.y, scale, a->sparkColor);
                paintScenePixel(_tft, p.x - 1, p.y, scale, a->sparkColor);
                paintScenePixel(_tft, p.x + 1, p.y, scale, a->sparkColor);
                paintScenePixel(_tft, p.x, p.y - 1, scale, a->sparkColor);
                paintScenePixel(_tft, p.x, p.y + 1, scale, a->sparkColor);
                break;
            }
            int base = a->sparkEvery10 ? a->sparkEvery10 * 10 : 150;
            s_nextSpark = now + base + animRange(base * 3);
        }
        for (auto& s : s_sparks) {
            if (!s.on || (int32_t)(now - s.until) < 0) continue;
            if (s.stage == 0) {
                restoreScenePixel(_tft, sc, s.x - 1, s.y);
                restoreScenePixel(_tft, sc, s.x + 1, s.y);
                restoreScenePixel(_tft, sc, s.x, s.y - 1);
                restoreScenePixel(_tft, sc, s.x, s.y + 1);
                s.stage = 1;
                s.until = now + 150;
            } else {
                restoreScenePixel(_tft, sc, s.x, s.y);
                s.on = false;
            }
        }
    }

    // Ripple: sound waves light up ring by ring away from the speaker, then rest
    if (a->rippleCount && a->ripple && (int32_t)(now - s_nextRipple) >= 0) {
        if (s_rippleRing >= 0) drawRippleRing(_tft, sc, s_rippleRing, false);
        s_rippleRing++;
        if (s_rippleRing >= a->rippleRings) {
            s_rippleRing = -1;
            s_nextRipple = now + 350;
        } else {
            drawRippleRing(_tft, sc, s_rippleRing, true);
            s_nextRipple = now + 70;
        }
    }

    // Wiggle: a limb jiggles a pixel right, back, left, back
    if ((a->wiggleCountA || a->wiggleCountB) && (int32_t)(now - s_nextWiggle) >= 0) {
        switch (s_wigglePhase) {
            case 0: drawAnimPixels(_tft, sc, a->wiggleA, a->wiggleCountA, false); break;
            case 1: drawAnimPixels(_tft, sc, a->wiggleA, a->wiggleCountA, true); break;
            case 2: drawAnimPixels(_tft, sc, a->wiggleB, a->wiggleCountB, false); break;
            default: drawAnimPixels(_tft, sc, a->wiggleB, a->wiggleCountB, true); break;
        }
        s_wigglePhase = (s_wigglePhase + 1) & 3;
        s_nextWiggle = now + 120;
    }

    // Particles, stepped at 10 Hz
    if (!a->particleKind || (int32_t)(now - s_nextTick) < 0) return;
    s_nextTick = now + 100;
    int maxX = a->areaX + a->areaW - a->glyphW, maxY = a->areaY + a->areaH - a->glyphH;
    int alive = 0;
    for (auto& p : s_particles) {
        if (!p.alive) continue;
        switch (a->particleKind) {
            case 1: p.y16 -= a->speed16; p.x16 += animRange(9) - 4; break;               // rise, wavering
            case 2: p.y16 += a->speed16; p.x16 += p.vx16 + animRange(7) - 3; break;      // fall, fluttering
            default:                                                                       // drift
                if (animRange(8) == 0) p.vx16 = animRange(2 * a->speed16 + 1) - a->speed16;
                if (animRange(8) == 0) p.vy16 = animRange(a->speed16 + 1) - a->speed16 / 2;
                p.x16 += p.vx16; p.y16 += p.vy16; break;
        }
        p.age++;
        int nx = p.x16 >> 4, ny = p.y16 >> 4;
        bool out = nx < a->areaX || ny < a->areaY || nx > maxX || ny > maxY || p.age > 150;
        if (p.drawn && (out || nx != p.px || ny != p.py)) {
            glyphPixels(_tft, sc, p.px, p.py, true, 0);
            p.drawn = false;
        }
        if (out) { p.alive = false; continue; }
        if (!p.drawn) {
            glyphPixels(_tft, sc, nx, ny, false, p.color);
            p.px = nx; p.py = ny; p.drawn = true;
        }
        alive++;
    }
    if (s_spawnCountdown > 0) { s_spawnCountdown--; return; }
    if (alive >= a->maxParticles) return;
    for (auto& p : s_particles) {
        if (p.alive) continue;
        int x = a->emitX + animRange(a->emitW), y = a->emitY + animRange(a->emitH);
        p.alive = true; p.drawn = false; p.age = 0;
        p.x16 = x << 4; p.y16 = y << 4;
        p.vx16 = animRange(5) - 2;
        p.vy16 = 0;
        p.color = a->colors[animRange(a->colorCount > 0 ? a->colorCount : 1)];
        break;
    }
    s_spawnCountdown = a->spawnEvery;
}

// Expensive (a whole screen of SPI): only call on a full repaint.
void DisplayManager::drawScene(int sceneIndex) {
    const KubiScene* sc = sceneAt(sceneIndex);
    if (!sc) return;

    uint8_t scale = sc->scale > 0 ? sc->scale : 1;
    int totalPixels = sc->width * sc->height;
    int curPixel = 0;
    bool keepIndex = sc->width == ANIM_W && sc->height == ANIM_H;
    s_idxScene = nullptr;

    // Each run becomes one fillRect per source row it covers, instead of
    // scale*scale drawPixel calls per pixel (each its own SPI transaction).
    for (uint32_t i = 0; i + 1 < sc->rleLength && curPixel < totalPixels; i += 2) {
        int count = pgm_read_byte(&sc->rleData[i]);
        uint8_t colorIdx = pgm_read_byte(&sc->rleData[i + 1]);
        if (colorIdx >= sc->paletteSize) colorIdx = 0;
        uint16_t color = pgm_read_word(&sc->palette[colorIdx]);

        while (count > 0 && curPixel < totalPixels) {
            int px = curPixel % sc->width;
            int py = curPixel / sc->width;
            int span = sc->width - px;
            if (span > count) span = count;
            _tft.fillRect(px * scale, py * scale, span * scale, scale, color);
            if (keepIndex) memset(&s_sceneIdx[curPixel], colorIdx, span);
            curPixel += span;
            count -= span;
        }
    }
    if (keepIndex && curPixel == totalPixels) {
        s_idxScene = sc;
        resetSceneAnimation(millis());
    }
}

void DisplayManager::drawMascotFace(float temperature, int hourOfDay, int sceneIndex, bool showHud) {
    const KubiScene* sc = sceneIndex >= 0 ? sceneAt(sceneIndex) : nullptr;
    if (!sc) {
        // No scenes compiled in: the vector mascot
        drawMascotFace(temperature, hourOfDay);
        return;
    }

    int w = _tft.width();
    int h = _tft.height();
    int cx = w / 2;

    // The scene is static: repaint it only when another one is picked. It
    // covers the screen in the normal portrait pose, so skip the clear then.
    if (_scene == SCENE_MASCOT_DIORAMA && sc != _drawnDiorama) invalidate();
    uint8_t scale = sc->scale > 0 ? sc->scale : 1;
    bool covers = sc->width * scale >= w && sc->height * scale >= h;
    bool full = beginScene(SCENE_MASCOT_DIORAMA, !covers);

    if (full) {
        drawScene(sceneIndex);
        _drawnDiorama = sc;
    }

    // Bottom temperature badge pill, repainted whole when the reading changes
    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f °C", temperature);
    if (showHud && (full || strcmp(tempBuf, _drawnTemp) != 0)) {
        _tft.fillRoundRect(cx - 50, h - 36, 100, 28, 8, 0x18C3);
        _tft.drawRoundRect(cx - 50, h - 36, 100, 28, 8, TFT_GOLD);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(TFT_GOLD, 0x18C3);
        _tft.drawString(tempBuf, cx, h - 22, 2);
        strncpy(_drawnTemp, tempBuf, sizeof(_drawnTemp) - 1);
        _drawnTemp[sizeof(_drawnTemp) - 1] = '\0';
    }

    animateScene(sc, millis());
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
