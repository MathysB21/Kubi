#pragma once
// Battery state of charge from the cell voltage.
//
// The pack is two INR18650-26E cells in parallel, read on GPIO 34 through
// R1 (B+ to SENSE, measured 9.28 kohm) over R2 (SENSE to GND, 9.42 kohm).
// Header-only and free of Arduino calls so the simulator can share it.
#include <math.h>
#include <stdint.h>

// SENSE = V_cell * R2 / (R1 + R2); bench check 2026-11-11: 1.99 V at SENSE.
static const float BATTERY_DIVIDER_RATIO = 9.42f / (9.28f + 9.42f);

// Resting Li-ion discharge curve (volts -> percent), light load. The cube draws
// ~0.3 A, which reads a little low; the smoothing in the caller hides the WiFi
// dips. Below 3.3 V the cell is effectively empty well before the charger's
// protection cuts off at ~2.5 V.
inline float batteryPercentFromVoltsExact(float v) {
    static const float VOLTS[]   = { 3.30f, 3.61f, 3.69f, 3.71f, 3.73f, 3.75f, 3.77f, 3.79f, 3.80f, 3.82f, 3.84f,
                                     3.85f, 3.87f, 3.91f, 3.95f, 3.98f, 4.02f, 4.08f, 4.11f, 4.15f, 4.20f };
    static const float PERCENT[] = { 0,     5,     10,    15,    20,    25,    30,    35,    40,    45,    50,
                                     55,    60,    65,    70,    75,    80,    85,    90,    95,    100 };
    const int n = sizeof(VOLTS) / sizeof(VOLTS[0]);
    if (v <= VOLTS[0]) return 0.0f;
    if (v >= VOLTS[n - 1]) return 100.0f;
    for (int i = 1; i < n; i++) {
        if (v < VOLTS[i]) {
            float t = (v - VOLTS[i - 1]) / (VOLTS[i] - VOLTS[i - 1]);
            return PERCENT[i - 1] + t * (PERCENT[i] - PERCENT[i - 1]);
        }
    }
    return 100.0f;
}

inline int batteryPercentFromVolts(float v) {
    return (int)(batteryPercentFromVoltsExact(v) + 0.5f);
}

// The ESP32 cannot see the charger's LEDs, so charging is read from the cell:
// plugging USB-C in lifts it ~0.1 V at once, unplugging drops it back. A fast
// and a slow average of the voltage catch those steps; a rise over 10 minutes
// catches a boot while already charging. Full: the charger has held the cell
// at >= 4.17 V for 5 minutes (TP4056 stops at 4.2 V).
enum BatteryState : uint8_t { BATTERY_DRAINING = 0, BATTERY_CHARGING = 1, BATTERY_FULL = 2 };

class BatteryMonitor {
public:
    // One raw cell voltage per call, every couple of seconds
    void update(float v, uint32_t nowMs) {
        if (!_init) {
            _fast = _slow = v;
            _init = true;
            _stateSince = _minuteStart = nowMs;
            _shown = batteryPercentFromVolts(v);
            return;
        }
        _fast += 0.4f * (v - _fast);
        _slow += 0.011f * (v - _slow); // ~3 min at one read per 2 s

        float step = _fast - _slow;
        if (step > 0.05f && _state == BATTERY_DRAINING) setState(BATTERY_CHARGING, nowMs);
        else if (step < -0.05f && _state != BATTERY_DRAINING) setState(BATTERY_DRAINING, nowMs);

        if (_state == BATTERY_CHARGING) {
            if (_slow >= 4.17f) {
                if (!_highSince) _highSince = nowMs;
                else if (nowMs - _highSince >= 5UL * 60UL * 1000UL) setState(BATTERY_FULL, nowMs);
            } else {
                _highSince = 0;
            }
        }

        // The charger lifts the reading while it pushes current in
        float exact = _state == BATTERY_FULL ? 100.0f
                    : batteryPercentFromVoltsExact(_slow - (_state == BATTERY_CHARGING ? 0.04f : 0.0f));
        _exact = exact;

        // Like a phone: draining only counts down, charging only counts up,
        // and only once the average is clearly past the next whole number
        if (_resync) {
            if (fabsf(exact - _shown) >= 0.7f) _shown = (int)(exact + 0.5f);
            if (nowMs - _stateSince > 10UL * 60UL * 1000UL) _resync = false;
        } else if (_state == BATTERY_DRAINING) {
            if (exact <= _shown - 0.7f) _shown = (int)(exact + 0.5f);
        } else if (exact >= _shown + 0.7f) {
            _shown = (int)(exact + 0.5f);
        }

        // One sample a minute for the trend and the time estimate
        if (nowMs - _minuteStart >= 60000UL) {
            _minuteStart = nowMs;
            _hist[_histHead] = exact;
            _vhist[_histHead] = _slow;
            _histHead = (_histHead + 1) % HIST;
            if (_histCount < HIST) _histCount++;
            // Booted while plugged in: no step to see, but the cell keeps rising
            if (_state == BATTERY_DRAINING && _histCount >= 10 && _slow - vAgo(9) > 0.03f) {
                setState(BATTERY_CHARGING, nowMs);
            }
        }
    }

    float voltage() const { return _slow; }
    int percent() const { return _shown; }
    BatteryState state() const { return _state; }

    // Minutes to empty (draining) or to full (charging); -1 while it can't tell yet
    int minutesLeft() const {
        if (_state == BATTERY_FULL) return 0;
        int k = _histCount - 1;
        if (k > 30) k = 30;
        if (k < 10) return -1;
        float rate = (_hist[idx(0)] - _hist[idx(k)]) / k; // percent per minute
        float left;
        if (_state == BATTERY_DRAINING) {
            if (rate > -0.005f) return -1;
            left = _exact / -rate;
        } else {
            if (rate < 0.01f) return -1;
            left = (100.0f - _exact) / rate;
        }
        return left > 5999.0f ? 5999 : (int)left;
    }

private:
    static const int HIST = 32;
    bool _init = false, _resync = false;
    float _fast = 0, _slow = 0, _exact = 100;
    int _shown = 100;
    BatteryState _state = BATTERY_DRAINING;
    uint32_t _stateSince = 0, _highSince = 0, _minuteStart = 0;
    float _hist[HIST] = {}, _vhist[HIST] = {};
    int _histHead = 0, _histCount = 0;

    int idx(int ago) const { return ((_histHead - 1 - ago) % HIST + HIST) % HIST; }
    float vAgo(int ago) const { return _vhist[idx(ago)]; }

    void setState(BatteryState s, uint32_t nowMs) {
        _state = s;
        _stateSince = nowMs;
        _highSince = 0;
        _histCount = 0;  // the old trend says nothing about the new direction
        _resync = true;  // let the shown percent follow the reading for a while
    }
};
