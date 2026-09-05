#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_BMP280.h>

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
    KubiGesture getRecentGesture(); // Returns and clears last gesture
    float getTemperature() const { return _temperature; }

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

    // Shake detection tracking
    int _shakeCount;
    int _lastSignX;
    uint32_t _shakeWindowStart;

    // Tap debouncing
    uint32_t _lastTapTime;

    void processMotion(float x, float y, float z);
    void updateFace(float x, float y, float z);
};

extern SensorManager sensors;
