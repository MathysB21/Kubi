#include "SensorManager.h"
#include "FaceMap.h"
#include <math.h>

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// m/s^2 change between samples that counts as "someone moved the cube".
// Tune on hardware: ADXL345 at 16 g is ~0.3 m/s^2 per LSB.
#define MOTION_WAKE_DELTA 1.5f

// TAPS (measured in the assembled cube, 2026-11-17, /api/motion-log; the IMU
// sits on padded standoffs, which soften taps a lot):
//   soft taps peak at a deltaMag of 2.4-5, firm ones 5-12, the hardest 28;
//   at rest the noise is ~0.12 (max 0.16 over half a second).
// Putting the cube down or rolling it peaks at 2-12 too, so the size of the
// spike cannot tell a tap from handling. What can: a tap lands on a cube that
// has been still. In the half second before every recorded tap, nothing moved
// more than 0.16; before every handling spike something moved 0.63 or more.
// So a tap is a spike >= TAP_MIN_DELTA after TAP_STILL_MS below STILL_DELTA.
#define TAP_MIN_DELTA 2.2f
#define STILL_DELTA   0.4f
#define TAP_STILL_MS  500
// A tap's first sample can be a small rise (2.0) before its peak (3.0) the
// next sample: the peak counts if it comes this soon after the burst began.
#define TAP_ONSET_MS  80
// A tap rings on for ~half a second, but above MOTION_WAKE_DELTA (1.5) only for
// ~200 ms. It is only reported after TAP_SETTLE_MS without such movement; if it
// is still moving that much TAP_MAX_RING_MS after the spike it was a grab.
#define TAP_SETTLE_MS   100
#define TAP_MAX_RING_MS 600

// A hard tap jolts gravity off its axis for one sample (20 ms), which looks
// like a face change. Only a new pose held this long is a real roll that
// should drop a pending tap.
#define POSE_MOVE_MS 100

// The rebound of a tap is a second shake reversal within a sample or three;
// only a reversal later than this cancels a pending tap as the start of a shake.
#define TAP_REBOUND_MS 80

// Desk slam (the "ouch" chime): |reading| above this. Hard taps reached 26.6
// and a shake 26.8 in the cube; 35 let hard taps say ouch.
#define SLAM_TOTAL 50.0f

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
      _pressure(0.0f),
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

            // Recorder: start on request, stop when full or out of time
            if (_recRequested) {
                _recRequested = false;
                if (!_recBuf) _recBuf = (MotionSample*)malloc(sizeof(MotionSample) * MOTION_LOG_MAX);
                if (_recBuf) {
                    _recCount = 0;
                    _recStart = now;
                    _recState = 1;
                    Serial.printf("[MOTION LOG] Recording %u s\n", (unsigned)_recSeconds);
                }
            }
            if (_recState == 1 && (now - _recStart >= (uint32_t)_recSeconds * 1000UL || _recCount >= MOTION_LOG_MAX)) {
                _recState = 2;
                Serial.printf("[MOTION LOG] Done, %d samples\n", (int)_recCount);
            }
            _recFlags = validRead ? 0 : MREC_BAD_READ;
            if (!validRead) recordSample(x, y, z, 0, sqrtf(x * x + y * y + z * z));

            if (validRead) {
                {
                    // What the detectors are about to see (same maths as processMotion)
                    float dx = x - _prevX, dy = y - _prevY, dz = z - _prevZ;
                    float delta = sqrtf(dx * dx + dy * dy + dz * dz);
                    float total = sqrtf(x * x + y * y + z * z);
                    processMotion(x, y, z);
                    updateFace(x, y, z);
                    recordSample(x, y, z, delta, total);
                }

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
            _pressure = _bmp.readPressure() / 100.0f;
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

    // Did this burst of movement start on a still cube, just now? (taps only
    // land on a still cube; see TAPS)
    if (deltaMag >= STILL_DELTA) {
        if (now - _lastMovingTime >= TAP_STILL_MS) _burstStart = now;
        _lastMovingTime = now;
    }
    bool wasStill = now - _burstStart <= TAP_ONSET_MS;

    // 0. Wake-on-motion: anything well above ADXL345 noise.
    // deltaMag spans two samples (see the ordering quirk in loop()).
    if (deltaMag > MOTION_WAKE_DELTA) {
        _lastMotionTime = now;
    }

    // A tap that survived its confirmation window without shaking, and after
    // which the cube went still again, is real; still moving = a grab
    if (_tapPending && now - _tapPendingSince >= TAP_CONFIRM_MS) {
        if (now - _lastMotionTime >= TAP_SETTLE_MS) {
            _tapPending = false;
            _recentGesture = GESTURE_TAP;
            _recFlags |= MREC_TAP;
            Serial.println("[GESTURE] >>> GENTLE TAP DETECTED <<<");
        } else if (now - _tapPendingSince > TAP_MAX_RING_MS) {
            _tapPending = false;
            _recFlags |= MREC_TAP_CANCELLED;
        }
    }

    // 1. Desk Slam Detection: violent impulse (SLAM_TOTAL, ~5 g)
    if (totalMag > SLAM_TOTAL && (now - _lastTapTime > 600)) {
        if (_tapPending) _recFlags |= MREC_TAP_CANCELLED;
        _tapPending = false; // its leading edge is not a tap
        _recentGesture = GESTURE_SLAM;
        _recFlags |= MREC_SLAM;
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
            _recFlags |= MREC_REVERSAL;
            if (shakeHoldOff) {
                // Still the same bout of shaking: keep holding off
                _lastShakeTime = now;
            } else if (now - _shakeWindowStart[a] > 500) {
                _shakeCount[a] = 1;
                _shakeWindowStart[a] = now;
            } else {
                _shakeCount[a]++;
                // A second reversal is the start of a shake, unless it is the
                // tap's own rebound (within TAP_REBOUND_MS of the tap)
                if (_tapPending && now - _tapPendingSince > TAP_REBOUND_MS) {
                    _recFlags |= MREC_TAP_CANCELLED;
                    _tapPending = false;
                }
                if (_shakeCount[a] >= 3) {
                    if (_tapPending) _recFlags |= MREC_TAP_CANCELLED;
                    _tapPending = false; // the shake's first jolt was not a tap
                    _recentGesture = GESTURE_SHAKE;
                    _recFlags |= MREC_SHAKE;
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

    // 3. Tap Detection: a sharp spike on a cube that was still (see TAPS at the
    // top), held as a candidate until TAP_CONFIRM_MS shows it was not the
    // start of a shake
    if (deltaMag >= TAP_MIN_DELTA && wasStill && (now - _lastTapTime > 350) && !shaking) {
        _tapPending = true;
        _recFlags |= MREC_TAP_CANDIDATE;
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
    // here made it get logged but never acted on. And only a real move: a hard
    // tap knocks gravity off its axis for one sample, so a pending tap is
    // dropped once another pose has held POSE_MOVE_MS, not on the first flip.
    if (pose != _candidatePose) {
        _candidatePose = pose;
        _candidateSince = millis();
        _lastTapTime = millis(); // no NEW tap candidates while the pose flips
    } else if (faceForPose(_candidatePose) != _activeFace && millis() - _candidateSince > POSE_MOVE_MS) {
        dropTap(); // being rolled to another face (or off any face)
    }
    if (pose == _candidatePose && millis() - _candidateSince > _faceSettleMs) {
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
    if (_tapPending || _recentGesture == GESTURE_TAP) _recFlags |= MREC_TAP_CANCELLED;
    _tapPending = false;
    if (_recentGesture == GESTURE_TAP) _recentGesture = GESTURE_NONE;
}

KubiGesture SensorManager::getRecentGesture() {
    KubiGesture g = _recentGesture;
    _recentGesture = GESTURE_NONE;
    return g;
}

void SensorManager::recordSample(float x, float y, float z, float delta, float total) {
    if (_recState != 1 || !_recBuf || _recCount >= MOTION_LOG_MAX) return;
    auto c = [](float v) -> int16_t {
        float s = v * 100.0f;
        return (int16_t)(s > 32767.f ? 32767.f : (s < -32768.f ? -32768.f : s));
    };
    MotionSample& m = _recBuf[_recCount];
    m.t = millis() - _recStart;
    m.x = c(x); m.y = c(y); m.z = c(z);
    m.delta = c(delta);
    m.total = c(total);
    m.flags = _recFlags;
    _recCount = _recCount + 1;
}
