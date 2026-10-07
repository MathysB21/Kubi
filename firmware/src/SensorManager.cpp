#include "SensorManager.h"
#include "FaceMap.h"
#include <math.h>

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// m/s^2 change between samples that counts as "someone moved the cube".
// Tap is 7.0. Tune on hardware: ADXL345 at 16 g is ~0.3 m/s^2 per LSB.
#define MOTION_WAKE_DELTA 1.5f

// A tap is only reported after this long without shaking: the first jolt of a
// shake looks exactly like a tap, and a tap on Pomodoro would pause the timer
// that the shake is about to skip.
#define TAP_CONFIRM_MS 250

// One bout of shaking is one shake: reversals keep extending this hold-off,
// and a new shake needs this long of quiet first.
#define SHAKE_QUIET_MS 600

// A failed I2C read (loose contact) returns ~0 on every axis. Gravity never
// vanishes on a desk, so a sample this small is a bad read, not motion.
#define MIN_VALID_MAG 2.0f

// Shakes are read from the reading minus a slow gravity estimate. At 50 Hz an
// alpha of 0.04 is a ~0.5 s time constant: it follows a roll to a new face
// within a second but barely moves during a 3-6 Hz shake.
#define GRAVITY_ALPHA 0.04f
#define SHAKE_THRESHOLD 3.0f // m/s^2 of dynamic acceleration for a reversal

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
      _gravity{ 0, 0, 9.8f },
      _gravityInit(false),
      _shakeCount{ 0, 0, 0 },
      _shakeSign{ 0, 0, 0 },
      _shakeWindowStart{ 0, 0, 0 },
      _lastShakeTime(0),
      _lastTapTime(0),
      _tapPending(false),
      _tapPendingSince(0),
      _lastMotionTime(0),
      _candidatePose(AXIS_Z * 2), // +Z, as before
      _candidateSince(0),
      _faceSettleMs(FACE_SETTLE_MS) {}

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

            // Drop bad reads entirely: no gesture, no face change, and they
            // never become _prev/_last, so the next good sample is no jolt.
            bool validRead = sqrtf(x * x + y * y + z * z) >= MIN_VALID_MAG;

            if (validRead) {
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

    // 0. Wake-on-motion: anything well above ADXL345 noise but far below a tap.
    // deltaMag spans two samples (see the ordering quirk in loop()).
    if (deltaMag > MOTION_WAKE_DELTA) {
        _lastMotionTime = now;
    }

    // A tap that survived its confirmation window without shaking is real
    if (_tapPending && now - _tapPendingSince >= TAP_CONFIRM_MS) {
        _tapPending = false;
        _recentGesture = GESTURE_TAP;
        Serial.println("[GESTURE] >>> GENTLE TAP DETECTED <<<");
    }

    // 1. Desk Slam Detection: Violent impulse (>3.5G total magnitude)
    if (totalMag > 35.0f && (now - _lastTapTime > 600)) {
        _tapPending = false; // its leading edge is not a tap
        _recentGesture = GESTURE_SLAM;
        _lastTapTime = now;
        Serial.println("[GESTURE] >>> DESK SLAM DETECTED! <<<");
        return;
    }

    // 2. Shake Detection: rapid sign reversals of the dynamic acceleration
    // (reading minus a slow gravity estimate) on any axis. Raw X used to be
    // the signal, but standing upright gravity sits on X or Y (FaceMap.h), so
    // on Faces 1 and 3 X never crossed zero and a shake could not register.
    if (!_gravityInit) {
        _gravity[0] = x; _gravity[1] = y; _gravity[2] = z;
        _gravityInit = true;
    }
    float accel[3] = { x, y, z };
    bool shakeHoldOff = now - _lastShakeTime < SHAKE_QUIET_MS;
    bool shaking = shakeHoldOff;
    for (int a = 0; a < 3; a++) {
        float dynamic = accel[a] - _gravity[a];
        _gravity[a] += GRAVITY_ALPHA * dynamic;

        int sign = (dynamic > SHAKE_THRESHOLD) ? 1 : ((dynamic < -SHAKE_THRESHOLD) ? -1 : 0);
        if (sign != 0 && sign != _shakeSign[a]) {
            _shakeSign[a] = sign;
            if (shakeHoldOff) {
                // Still the same bout of shaking: keep holding off
                _lastShakeTime = now;
            } else if (now - _shakeWindowStart[a] > 500) {
                _shakeCount[a] = 1;
                _shakeWindowStart[a] = now;
            } else {
                _shakeCount[a]++;
                _tapPending = false; // a second reversal: this is a shake, not a tap
                if (_shakeCount[a] >= 3) {
                    _recentGesture = GESTURE_SHAKE;
                    for (int b = 0; b < 3; b++) _shakeCount[b] = 0;
                    _lastShakeTime = now;
                    _lastTapTime = now;
                    Serial.println("[GESTURE] >>> SHAKE DETECTED! <<<");
                    return;
                }
            }
        }
        if (_shakeCount[a] >= 2 && now - _shakeWindowStart[a] <= 500) shaking = true;
    }

    // 3. Gentle Tap Detection: Sharp transient acceleration change, held as a
    // candidate until TAP_CONFIRM_MS shows it was not the start of a shake
    if (deltaMag > 7.0f && deltaMag < 30.0f && (now - _lastTapTime > 350) && !shaking) {
        _tapPending = true;
        _tapPendingSince = now;
        _lastTapTime = now;
    }
}

void SensorManager::updateFace(float x, float y, float z) {
    // Which resting position is this? The pose -> face mapping lives in FaceMap.h.
    int pose = classifyPose(x, y, z);

    // Debounce orientation: must remain stable for _faceSettleMs before switching face
    // Rolling the cube jolts like a tap, so orientation changes drop taps. Only
    // taps: a hard shake flips the dominant axis too, and erasing the shake
    // here made it get logged but never acted on.
    if (pose != _candidatePose) {
        _candidatePose = pose;
        _candidateSince = millis();
        _lastTapTime = millis(); // Suppress false tap detection during orientation flips
        dropTap();
    } else if (millis() - _candidateSince > _faceSettleMs) {
        int face = faceForPose(_candidatePose);
        if (face >= 0 && _activeFace != face) {
            _activeFace = face;
            _lastTapTime = millis(); // Suppress tap when face settles
            dropTap();
            Serial.printf("[ORIENTATION] Cube resting on Face %d UP\n", _activeFace + 1);
        }
    }
}

void SensorManager::dropTap() {
    _tapPending = false;
    if (_recentGesture == GESTURE_TAP) _recentGesture = GESTURE_NONE;
}

KubiGesture SensorManager::getRecentGesture() {
    KubiGesture g = _recentGesture;
    _recentGesture = GESTURE_NONE;
    return g;
}
