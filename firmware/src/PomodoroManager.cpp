#include "PomodoroManager.h"
#include "AudioManager.h"
#include <Preferences.h>

PomodoroManager pomodoro;

PomodoroManager::PomodoroManager()
    : _phase(POMO_WORK),
      _remainingSeconds(25 * 60),
      _totalSeconds(25 * 60),
      _isStarted(false),
      _isPaused(false),
      _hasChimed(false),
      _completedCycles(0),
      _focusMinutes(25),
      _shortBreakMinutes(5),
      _longBreakMinutes(15),
      _cycleTarget(4),
      _colorWork(0xFCE0),       // Default Amber (#F59E0B)
      _colorShortBreak(0x15D0),  // Default Emerald (#10B981)
      _colorLongBreak(0x0D39)   // Default Sky (#0EA5E9)
{}

void PomodoroManager::init() {
    Preferences prefs;
    prefs.begin("kubi_settings", false);
    _focusMinutes      = prefs.getInt("pomoFocus", 25);
    _shortBreakMinutes = prefs.getInt("pomoBreak", 5);
    _longBreakMinutes  = prefs.getInt("pomoLong", 15);
    _cycleTarget       = prefs.getInt("pomoCycles", 4);

    String hexWork  = prefs.getString("pomoCW", "#F59E0B");
    String hexShort = prefs.getString("pomoCS", "#10B981");
    String hexLong  = prefs.getString("pomoCL", "#0EA5E9");
    setColors(hexWork, hexShort, hexLong);

    prefs.end();

    _phase = POMO_WORK;
    _completedCycles = 0;
    _isStarted = false;
    _isPaused = false;
    _hasChimed = false;
    updatePhaseDurations();
}

void PomodoroManager::updatePhaseDurations() {
    if (_phase == POMO_WORK) {
        _totalSeconds = _focusMinutes * 60;
    } else if (_phase == POMO_SHORT_BREAK) {
        _totalSeconds = _shortBreakMinutes * 60;
    } else {
        _totalSeconds = _longBreakMinutes * 60;
    }
    _remainingSeconds = _totalSeconds;
}

const char* PomodoroManager::getPhaseName() const {
    switch (_phase) {
        case POMO_WORK: return "FOCUS";
        case POMO_SHORT_BREAK: return "SHORT BREAK";
        case POMO_LONG_BREAK: return "LONG BREAK";
        default: return "FOCUS";
    }
}

uint16_t PomodoroManager::getPhaseColor() const {
    switch (_phase) {
        case POMO_WORK: return _colorWork;
        case POMO_SHORT_BREAK: return _colorShortBreak;
        case POMO_LONG_BREAK: return _colorLongBreak;
        default: return _colorWork;
    }
}

void PomodoroManager::tick() {
    if (_isStarted && !_isPaused && _remainingSeconds > 0) {
        _remainingSeconds--;
        if (_remainingSeconds == 0 && !_hasChimed) {
            _hasChimed = true;
            // If the completed focus session leads to a LONG BREAK, play unique long chime
            if (_phase == POMO_WORK && (_completedCycles + 1 >= _cycleTarget)) {
                audio.playChime(CHIME_POMODORO_LONG_BREAK);
                Serial.printf("[POMODORO] %s complete! All sessions done -> LONG BREAK chime ringing!\n", getPhaseName());
            } else {
                audio.playChime(CHIME_POMODORO_DONE);
                Serial.printf("[POMODORO] %s complete! Chime ringing.\n", getPhaseName());
            }
        }
    }
}

void PomodoroManager::handleTap() {
    if (_hasChimed) {
        // Dismiss active chime and transition to next phase
        audio.stop();
        _hasChimed = false;
        advancePhase();
        Serial.println("[POMODORO] Tap dismissed chime -> advancing to next phase.");
    } else {
        // Start or Toggle Pause / Play
        togglePause();
        audio.playChime(CHIME_TAP_FEEDBACK);
    }
}

void PomodoroManager::handleShake() {
    // Shake: Skip to next phase immediately
    Serial.println("[POMODORO] Shake detected -> Skipping to next phase!");
    skipPhase();
}

void PomodoroManager::play() {
    _isStarted = true;
    _isPaused = false;
    Serial.println("[POMODORO] Resumed/Playing");
}

void PomodoroManager::pause() {
    _isPaused = true;
    Serial.println("[POMODORO] Paused");
}

void PomodoroManager::togglePause() {
    if (!_isStarted) {
        _isStarted = true;
        _isPaused = false;
        Serial.println("[POMODORO] Started from idle");
    } else {
        _isPaused = !_isPaused;
        Serial.printf("[POMODORO] Pause toggled -> %s\n", _isPaused ? "PAUSED" : "RUNNING");
    }
}

void PomodoroManager::skipPhase() {
    if (_hasChimed) {
        audio.stop();
        _hasChimed = false;
    }
    advancePhase();
    if (_phase == POMO_LONG_BREAK) {
        audio.playChime(CHIME_POMODORO_LONG_BREAK);
    } else if (_phase == POMO_SHORT_BREAK) {
        audio.playChime(CHIME_POMODORO_DONE);
    } else {
        audio.playChime(CHIME_WAKE_PING);
    }
}

void PomodoroManager::resetCurrent() {
    if (_hasChimed) {
        audio.stop();
        _hasChimed = false;
    }
    _remainingSeconds = _totalSeconds;
    _isStarted = false;
    _isPaused = false;
    audio.playChime(CHIME_TAP_FEEDBACK);
    Serial.printf("[POMODORO] Current phase %s reset to %d mins\n", getPhaseName(), _totalSeconds / 60);
}

void PomodoroManager::advancePhase() {
    if (_phase == POMO_WORK) {
        _completedCycles++;
        if (_completedCycles >= _cycleTarget) {
            _completedCycles = 0;
            _phase = POMO_LONG_BREAK;
        } else {
            _phase = POMO_SHORT_BREAK;
        }
    } else {
        _phase = POMO_WORK;
    }

    updatePhaseDurations();
    _isStarted = true;
    _isPaused = false;
    Serial.printf("[POMODORO] Switched to %s (%d min) | Cycle %d of %d\n",
                  getPhaseName(), _totalSeconds / 60, _completedCycles + 1, _cycleTarget);
}

void PomodoroManager::setDurations(int focus, int shortBreak, int longBreak, int cycleTarget) {
    if (focus > 0) _focusMinutes = focus;
    if (shortBreak > 0) _shortBreakMinutes = shortBreak;
    if (longBreak > 0) _longBreakMinutes = longBreak;
    if (cycleTarget > 0) _cycleTarget = cycleTarget;

    updatePhaseDurations();

    // Persist to NVS
    Preferences prefs;
    prefs.begin("kubi_settings", false);
    prefs.putInt("pomoFocus", _focusMinutes);
    prefs.putInt("pomoBreak", _shortBreakMinutes);
    prefs.putInt("pomoLong",  _longBreakMinutes);
    prefs.putInt("pomoCycles", _cycleTarget);
    prefs.end();
}

void PomodoroManager::setColors(const String& hexWork, const String& hexShort, const String& hexLong) {
    _colorWork = hexToRGB565(hexWork);
    _colorShortBreak = hexToRGB565(hexShort);
    _colorLongBreak = hexToRGB565(hexLong);

    Preferences prefs;
    prefs.begin("kubi_settings", false);
    prefs.putString("pomoCW", hexWork);
    prefs.putString("pomoCS", hexShort);
    prefs.putString("pomoCL", hexLong);
    prefs.end();
}

String PomodoroManager::getColorWorkHex() const {
    return rgb565ToHex(_colorWork);
}

String PomodoroManager::getColorShortBreakHex() const {
    return rgb565ToHex(_colorShortBreak);
}

String PomodoroManager::getColorLongBreakHex() const {
    return rgb565ToHex(_colorLongBreak);
}

uint16_t PomodoroManager::hexToRGB565(const String& hex) {
    if (hex.length() < 7 || hex.charAt(0) != '#') return 0xFFFF; // Default white
    long number = strtol(hex.substring(1).c_str(), NULL, 16);
    uint8_t r = (number >> 16) & 0xFF;
    uint8_t g = (number >> 8) & 0xFF;
    uint8_t b = number & 0xFF;
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

String PomodoroManager::rgb565ToHex(uint16_t color) {
    uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
    uint8_t b = (color & 0x1F) * 255 / 31;
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    return String(buf);
}
