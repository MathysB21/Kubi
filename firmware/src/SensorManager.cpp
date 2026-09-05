#include "SensorManager.h"
#include <math.h>

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

SensorManager sensors;

SensorManager::SensorManager()
    : _adxl(12345),
      _bmp(),
      _adxlFound(false),
      _bmpFound(false),
      _lastX(0), _lastY(0), _lastZ(9.8),
      _prevX(0), _prevY(0), _prevZ(9.8),
      _lastPollTime(0),
      _lastTempReadTime(0),
      _temperature(21.5f),
      _activeFace(0),
      _recentGesture(GESTURE_NONE),
      _shakeCount(0),
      _lastSignX(0),
      _shakeWindowStart(0),
      _lastTapTime(0) {}

bool SensorManager::init() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(400000); // 400kHz I2C

    // 1. Initialize ADXL345 Accelerometer
    if (_adxl.begin(0x53)) {
        _adxlFound = true;
        _adxl.setRange(ADXL345_RANGE_16_G);
        Serial.println("[IMU] ADXL345 Accelerometer detected at 0x53");
    } else {
        Serial.println("[IMU] ADXL345 not found at 0x53, retrying 0x1D...");
        if (_adxl.begin(0x1D)) {
            _adxlFound = true;
            _adxl.setRange(ADXL345_RANGE_16_G);
            Serial.println("[IMU] ADXL345 detected at 0x1D");
        } else {
            Serial.println("[IMU] ERROR: ADXL345 not found!");
        }
    }

    // 2. Initialize BMP280 Temperature Sensor
    if (_bmp.begin(0x76)) {
        _bmpFound = true;
        Serial.println("[BMP] BMP280 detected at 0x76");
    } else if (_bmp.begin(0x77)) {
        _bmpFound = true;
        Serial.println("[BMP] BMP280 detected at 0x77");
    } else {
        Serial.println("[BMP] WARNING: BMP280 not found!");
    }

    return _adxlFound;
}

void SensorManager::loop() {
    uint32_t now = millis();

    // 1. Poll Accelerometer at ~50Hz (every 20ms)
    if (now - _lastPollTime >= 20) {
        _lastPollTime = now;

        if (_adxlFound) {
            sensors_event_t event;
            _adxl.getEvent(&event);

            float x = event.acceleration.x;
            float y = event.acceleration.y;
            float z = event.acceleration.z;

            processMotion(x, y, z);
            updateFace(x, y, z);

            _prevX = _lastX;
            _prevY = _lastY;
            _prevZ = _lastZ;

            _lastX = x;
            _lastY = y;
            _lastZ = z;
        }
    }

    // 2. Poll Temperature periodically (~every 2 seconds)
    if (now - _lastTempReadTime >= 2000) {
        _lastTempReadTime = now;
        if (_bmpFound) {
            _temperature = _bmp.readTemperature();
        }
    }
}

void SensorManager::processMotion(float x, float y, float z) {
    uint32_t now = millis();

    float dx = x - _prevX;
    float dy = y - _prevY;
    float dz = z - _prevZ;
    float deltaMag = sqrt(dx * dx + dy * dy + dz * dz);
    float totalMag = sqrt(x * x + y * y + z * z);

    // 1. Desk Slam Detection: Violent impulse (>3.5G total magnitude)
    if (totalMag > 35.0f && (now - _lastTapTime > 600)) {
        _recentGesture = GESTURE_SLAM;
        _lastTapTime = now;
        Serial.println("[GESTURE] >>> DESK SLAM DETECTED! <<<");
        return;
    }

    // 2. Shake Detection: Rapid sign reversals in X/Y axis
    int currentSignX = (x > 3.0f) ? 1 : ((x < -3.0f) ? -1 : 0);
    if (currentSignX != 0 && currentSignX != _lastSignX) {
        if (now - _shakeWindowStart > 500) {
            _shakeCount = 1;
            _shakeWindowStart = now;
        } else {
            _shakeCount++;
            if (_shakeCount >= 3) {
                _recentGesture = GESTURE_SHAKE;
                _shakeCount = 0;
                _lastTapTime = now;
                Serial.println("[GESTURE] >>> SHAKE DETECTED! <<<");
                return;
            }
        }
        _lastSignX = currentSignX;
    }

    // 3. Gentle Tap Detection: Sharp transient acceleration change
    if (deltaMag > 7.0f && deltaMag < 30.0f && (now - _lastTapTime > 350)) {
        _recentGesture = GESTURE_TAP;
        _lastTapTime = now;
        Serial.println("[GESTURE] >>> GENTLE TAP DETECTED <<<");
    }
}

void SensorManager::updateFace(float x, float y, float z) {
    static int candidateFace = 0;
    static uint32_t faceCandidateStartTime = 0;

    int detected = 0;
    float absX = fabs(x);
    float absY = fabs(y);
    float absZ = fabs(z);

    // Determine dominant axis
    if (absZ >= absX && absZ >= absY) {
        detected = (z > 0) ? 0 : 4; // Face 1 (Clock) or Inverted
    } else if (absX >= absY && absX >= absZ) {
        detected = (x > 0) ? 1 : 3; // Face 2 (Pomodoro) or Face 4 (Schedule)
    } else {
        detected = (y > 0) ? 2 : 5; // Face 3 (Mascot) or Bottom
    }

    // Debounce orientation: Must remain stable for 400ms before switching face
    if (detected != candidateFace) {
        candidateFace = detected;
        faceCandidateStartTime = millis();
        _lastTapTime = millis(); // Suppress false tap detection during orientation flips
        _recentGesture = GESTURE_NONE;
    } else if (millis() - faceCandidateStartTime > 400) {
        if (_activeFace != candidateFace && candidateFace <= 3) {
            _activeFace = candidateFace;
            _lastTapTime = millis(); // Suppress tap when face settles
            _recentGesture = GESTURE_NONE;
            Serial.printf("[ORIENTATION] Cube resting on Face %d UP\n", _activeFace + 1);
        }
    }
}

KubiGesture SensorManager::getRecentGesture() {
    KubiGesture g = _recentGesture;
    _recentGesture = GESTURE_NONE;
    return g;
}
