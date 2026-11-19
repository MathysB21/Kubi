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
// DISCONNECTED: the divider reads next to nothing, so the cells are not on the
// circuit at all. Seen 2026-11-19 (0.22 V): the pack ran flat overnight and
// the charger board's protection cut it off at ~2.5 V, while the ESP32's own
// USB kept Kubi running. Only the USB-C charger reconnects it.
enum BatteryState : uint8_t { BATTERY_DRAINING = 0, BATTERY_CHARGING = 1, BATTERY_FULL = 2,
                              BATTERY_DISCONNECTED = 3 };

// Below this the reading is not a cell (an empty 18650 rests at 2.5 V or more)
static const float BATTERY_PRESENT_V = 2.0f;
// Kubi shuts down (battery sleep, see main.cpp) once the draining average has
// stayed under this for BATTERY_EMPTY_HOLD_MS: 0 % on the curve below, about
// 3.4 V at rest, well clear of the protection's 2.5 V (the wiring guide's plan).
static const float BATTERY_EMPTY_V = 3.30f;
static const uint32_t BATTERY_EMPTY_HOLD_MS = 60000UL;

class BatteryMonitor {
public:
    // One raw cell voltage per call, every couple of seconds
    void update(float v, uint32_t nowMs) {
        // Cells off the circuit: report that, keep nothing from it, and start
        // afresh once they are back (the charger reconnects them)
        if (v < BATTERY_PRESENT_V) {
            _disconnected = true;
            _rawV = v;
            _init = false;
            _lowSince = 0;
            _empty = false;
            return;
        }
        _disconnected = false;
        // Warm-up: the first few readings after power-on (or reconnecting) only
        // seed the averages. Plugging in the charger reads its own 4.2 V for a
        // moment before the protection lets the cells back in (it showed 100 %
        // and then dropped to 0 %, 2026-11-19).
        if (!_init || _warmup > 0) {
            if (!_init) {
                _warmup = WARMUP_READS;
                _state = BATTERY_DRAINING;  // until a step or a rise says otherwise
                _histCount = 0;
                _resync = false;
                _highSince = 0;
            }
            _fast = _slow = v;
            _init = true;
            _sure = false;
            _stateSince = _minuteStart = nowMs;
            _shown = batteryPercentFromVolts(v);
            _warmup--;
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

        // The charger lifts the reading while it pushes current in: ~0.08 V on
        // this pack (2026-11-11: it read 92 % on charge and settled to 87 %)
        float exact = _state == BATTERY_FULL ? 100.0f
                    : batteryPercentFromVoltsExact(_slow - (_state == BATTERY_CHARGING ? 0.08f : 0.0f));
        _exact = exact;

        // Like a phone: draining only counts down, charging only counts up,
        // and only once the average is clearly past the next whole number.
        // After a plug-in or unplug the shown value eases over to the new
        // reading, a point every 2 minutes at most, instead of jumping.
        if (_resync) {
            if (fabsf(exact - _shown) >= 0.7f && nowMs - _lastEase >= 2UL * 60UL * 1000UL) {
                _shown += exact > _shown ? 1 : -1;
                _lastEase = nowMs;
            }
            if (nowMs - _stateSince > 20UL * 60UL * 1000UL) _resync = false;
        } else if (_state == BATTERY_DRAINING) {
            if (exact <= _shown - 0.7f) _shown = (int)(exact + 0.5f);
        } else if (exact >= _shown + 0.7f) {
            _shown = (int)(exact + 0.5f);
        }

        // One sample a minute for the trend and the time estimate. Not while
        // the slow average is still absorbing a plug-in/unplug step: those
        // minutes look like a very fast charge (seen 2026-11-11: "full in
        // 17 min" at 79 %, really about an hour).
        if (nowMs - _minuteStart >= 60000UL && (int32_t)(nowMs - _settleUntil) >= 0) {
            _minuteStart = nowMs;
            _hist[_histHead] = exact;
            _vhist[_histHead] = _slow;
            _histHead = (_histHead + 1) % HIST;
            if (_histCount < HIST) _histCount++;
            // Booted while plugged in: no step to see, but the cell keeps rising
            if (_state == BATTERY_DRAINING && _histCount >= 10 && _slow - vAgo(9) > 0.03f) {
                setState(BATTERY_CHARGING, nowMs);
            }
            // Not sure yet (just started): the cell's trend has to decide. Mid
            // charge it climbs only ~0.01 V per 10 min (the flat middle of the
            // curve, net ~0.7 A into 5.2 Ah), too slow for the rule above: on
            // 2026-11-19 a restart on the charger read "draining" for good. So
            // a clear fall settles it at 10 min, anything else waits for 30 min
            // of trend; and a cell held at 4.16 V+ is the charger's top-off.
            if (!_sure && _state == BATTERY_DRAINING) {
                if (_histCount >= 10 && _slow >= TOPOFF_V && vAgo(9) >= TOPOFF_V) {
                    setState(BATTERY_CHARGING, nowMs);
                } else if (_histCount >= 10 && _slow - vAgo(9) < -0.01f) {
                    _sure = true;                       // clearly falling: on battery
                } else if (_histCount >= 30) {
                    if (_slow - vAgo(29) > 0.012f) setState(BATTERY_CHARGING, nowMs);
                    else _sure = true;                  // half an hour without a rise
                }
            }
            learn();
        }

        // Empty: sure it is draining, and the average has stayed low a while
        if (_sure && _state == BATTERY_DRAINING && _slow < BATTERY_EMPTY_V) {
            if (!_lowSince) _lowSince = nowMs ? nowMs : 1;
            _empty = nowMs - _lowSince >= BATTERY_EMPTY_HOLD_MS;
        } else {
            _lowSince = 0;
            _empty = false;
        }
    }

    // Still in the warm-up: voltage and percent are not worth showing yet
    bool warming() const { return !_disconnected && (!_init || _warmup > 0); }
    // True once Kubi should shut down to spare the cells (see BATTERY_EMPTY_V)
    bool empty() const { return _empty; }
    // False just after power-on until a plug-in step, a rise, or ten minutes
    // of history says whether the charger is in: there is no charger signal
    bool sure() const { return _sure && !_disconnected; }

    // --- Learned rates (percent per hour) ---
    // A slow average of every half-hour trend this cube has seen, kept in NVS
    // by the caller, so an estimate is there straight after a restart and
    // one busy hour hardly moves it. 0 = nothing learned yet.
    void setLearnedRates(float drainPerHour, float chargePerHour) {
        if (drainPerHour >= MIN_DRAIN && drainPerHour <= MAX_DRAIN) _drainPerHour = drainPerHour;
        if (chargePerHour >= MIN_CHARGE && chargePerHour <= MAX_CHARGE) _chargePerHour = chargePerHour;
    }
    float learnedDrainPerHour() const { return _drainPerHour; }
    float learnedChargePerHour() const { return _chargePerHour; }

    float voltage() const { return _disconnected ? _rawV : _slow; }
    int percent() const { return _disconnected ? 0 : _shown; }
    BatteryState state() const { return _disconnected ? BATTERY_DISCONNECTED : _state; }

    // Minutes to empty (draining) or to full (charging); -1 while it can't tell yet.
    // The learned rate carries the estimate; the last half hour can move it
    // by up to half once there is enough of it.
    int minutesLeft() const {
        if (_disconnected) return -1;
        if (_state == BATTERY_FULL) return 0;
        if (!_sure) return -1; // not "0 min left" while it may be charging
        bool draining = _state == BATTERY_DRAINING;
        float learned = draining ? _drainPerHour : _chargePerHour;
        int k;
        float live = liveRatePerHour(k);
        float rate;
        if (learned > 0 && live > 0) {
            float w = (k > 30 ? 30 : k) / 60.0f;
            rate = learned * (1.0f - w) + live * w;
        } else if (learned > 0) {
            rate = learned;
        } else if (live > 0) {
            rate = live;
        } else {
            return -1;
        }
        float left = (draining ? _exact : 100.0f - _exact) / rate * 60.0f;
        return left > 5999.0f ? 5999 : (int)left;
    }

private:
    static const int HIST = 32;
    // Plausible bounds for this cube (5.2 Ah pack, ~0.3 A load, ~1 A charger)
    static constexpr float MIN_DRAIN = 0.3f, MAX_DRAIN = 50.0f;
    static constexpr float MIN_CHARGE = 1.0f, MAX_CHARGE = 100.0f;
    static constexpr float LEARN_WEIGHT = 0.01f; // per minute: ~1.7 h memory
    float _drainPerHour = 0, _chargePerHour = 0;
    static const uint32_t SETTLE_MS = 10UL * 60UL * 1000UL; // ~3 time constants of the slow average
    uint32_t _settleUntil = 0, _lastEase = 0;

    // Trend over up to the last 30 minutes in the current direction, percent
    // per hour (positive); 0 if there are under 10 minutes or it points the
    // wrong way. k = minutes used.
    float liveRatePerHour(int& k) const {
        k = _histCount - 1;
        if (k > 30) k = 30;
        if (k < 10 || _state == BATTERY_FULL) return 0;
        float perMin = (_hist[idx(0)] - _hist[idx(k)]) / k;
        float perHour = (_state == BATTERY_DRAINING ? -perMin : perMin) * 60.0f;
        float lo = _state == BATTERY_DRAINING ? MIN_DRAIN : MIN_CHARGE;
        float hi = _state == BATTERY_DRAINING ? MAX_DRAIN : MAX_CHARGE;
        return (perHour >= lo && perHour <= hi) ? perHour : 0;
    }

    void learn() {
        int k;
        float live = liveRatePerHour(k);
        if (live <= 0 || k < 30) return; // only full half hours teach it
        float& learned = _state == BATTERY_DRAINING ? _drainPerHour : _chargePerHour;
        learned = learned > 0 ? learned + LEARN_WEIGHT * (live - learned) : live;
    }
    static const int WARMUP_READS = 3;      // ~6 s at one read per 2 s
    // Under Kubi's ~0.3 A a full cell reads ~4.10 V; only the charger holds 4.16+
    static constexpr float TOPOFF_V = 4.16f;
    int _warmup = 0;
    bool _disconnected = false, _sure = false, _empty = false;
    float _rawV = 0;
    uint32_t _lowSince = 0;
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
        _sure = true;    // a step or a rise: now it knows
        _stateSince = nowMs;
        _settleUntil = nowMs + SETTLE_MS;
        _highSince = 0;
        _histCount = 0;  // the old trend says nothing about the new direction
        _resync = true;  // let the shown percent follow the reading for a while
    }
};
