#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_BMP280.h>

// How long a new resting position must hold before the face switches.
#define FACE_SETTLE_MS     400
#define FACE_SETTLE_LOCKED 2000  // while a game holds the face: "roll over and hold" to leave

enum KubiGesture {
    GESTURE_NONE = 0,
    GESTURE_TAP,
    GESTURE_SHAKE,
    GESTURE_SLAM
};

// Motion recorder, for tuning gestures on the real cube (/api/motion-log).
// One entry per accelerometer sample the detectors see, in centi-m/s^2, with
// what the detectors made of it.
enum MotionFlag : uint8_t {
    MREC_TAP_CANDIDATE = 1,   // passed the tap threshold, waiting TAP_CONFIRM_MS
    MREC_TAP           = 2,   // tap reported
    MREC_SLAM          = 4,   // slam reported (the ouch chime)
    MREC_SHAKE         = 8,   // shake reported
    MREC_REVERSAL      = 16,  // a shake sign reversal on some axis
    MREC_TAP_CANCELLED = 32,  // a pending tap dropped (shake or face change)
    MREC_BAD_READ      = 64   // sample discarded (I2C read near 0 g)
};
struct MotionSample {
    uint32_t t;                     // ms since the recording started
    int16_t  x, y, z;               // reading
    int16_t  delta;                 // the tap detector's deltaMag
    int16_t  total;                 // |reading|, the slam detector's input
    uint8_t  flags;                 // MotionFlag bits
};
#define MOTION_LOG_MAX 1500         // 30 s at 50 Hz, 24 KB, allocated on first use

class SensorManager {
public:
    SensorManager();

    // Recorder. request from any task; the sensor loop starts it on its next
    // sample. State: 0 idle, 1 recording, 2 done (log readable until the next
    // request). The log must only be read while the state is 2.
    void requestMotionLog(uint16_t seconds) { _recSeconds = seconds; _recRequested = true; }
    int  motionLogState() const { return _recState; }
    const MotionSample* motionLog(int& count) const { count = _recCount; return _recBuf; }

    bool init();
    void loop(); // Call frequently from Core 1 (~50-100Hz)

    int getActiveFace() const { return _activeFace; }
    // FACE_SETTLE_MS normally; FACE_SETTLE_LOCKED while the maze is being played
    void setFaceSettleTime(uint32_t ms) { _faceSettleMs = ms; }
    KubiGesture getRecentGesture(); // Returns and clears last gesture
    float getTemperature() const { return _temperature; }
    float getPressure() const { return _pressure; } // hPa, 0 until the first read
    // millis() of the last movement above the wake threshold (0 = none yet)
    uint32_t getLastMotionTime() const { return _lastMotionTime; }

    void getAcceleration(float &x, float &y, float &z) const {
        x = _lastX;
        y = _lastY;
        z = _lastZ;
    }

private:
    Adafruit_ADXL345_Unified _adxl;
    Adafruit_BMP280 _bmp;
    bool _adxlFound;
    bool _bmpFound;

    float _lastX, _lastY, _lastZ;
    float _prevX, _prevY, _prevZ;
    uint32_t _lastPollTime;
    uint32_t _lastTempReadTime;
    float _temperature;
    float _pressure;

    int _activeFace;
    KubiGesture _recentGesture;

    // Shake detection tracking, per axis, on the reading minus _gravity
    float _gravity[3];           // slow low-pass of the reading (gravity estimate)
    bool _gravityInit;
    int _shakeCount[3];
    int _shakeSign[3];
    uint32_t _shakeWindowStart[3];
    uint32_t _lastShakeTime;     // last reversal of the most recent reported shake

    // Tap debouncing; a tap waits TAP_CONFIRM_MS as a candidate before it counts
    uint32_t _lastTapTime;
    bool _tapPending;
    uint32_t _tapPendingSince;

    // Screen-wake motion tracking
    uint32_t _lastMotionTime;
    uint32_t _lastMovingTime = 0; // last sample with deltaMag >= STILL_DELTA (taps need stillness)
    uint32_t _burstStart = 0;     // first moving sample after TAP_STILL_MS of stillness

    // Orientation debounce (pose ids from FaceMap.h)
    int _candidatePose;
    uint32_t _candidateSince;
    uint32_t _faceSettleMs;

    // Recorder state (see requestMotionLog)
    volatile bool     _recRequested = false;
    volatile uint16_t _recSeconds = 0;
    volatile int      _recState = 0;
    MotionSample*     _recBuf = nullptr;
    volatile int      _recCount = 0;
    uint32_t          _recStart = 0;
    uint8_t           _recFlags = 0;       // flags gathered for the current sample
    void recordSample(float x, float y, float z, float delta, float total);

    void processMotion(float x, float y, float z);
    void updateFace(float x, float y, float z);
    void dropTap(); // forget a pending or unread tap (orientation changes)
};

extern SensorManager sensors;
