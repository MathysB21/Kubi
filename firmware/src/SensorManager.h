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

class SensorManager {
public:
    SensorManager();

    bool init();
    void loop(); // Call frequently from Core 1 (~50-100Hz)

    int getActiveFace() const { return _activeFace; }
    // FACE_SETTLE_MS normally; FACE_SETTLE_LOCKED while the maze is being played
    void setFaceSettleTime(uint32_t ms) { _faceSettleMs = ms; }
    KubiGesture getRecentGesture(); // Returns and clears last gesture
    float getTemperature() const { return _temperature; }
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

    // Orientation debounce (pose ids from FaceMap.h)
    int _candidatePose;
    uint32_t _candidateSince;
    uint32_t _faceSettleMs;

    void processMotion(float x, float y, float z);
    void updateFace(float x, float y, float z);
    void dropTap(); // forget a pending or unread tap (orientation changes)
};

extern SensorManager sensors;
