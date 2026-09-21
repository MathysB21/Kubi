#pragma once
#include <stdint.h>
#include <math.h>

// Which way up is the cube? One table decides it for the sensors (face
// detection), the display (screen rotation) and the simulator (/sim/inject face).

enum GravityAxis : uint8_t { AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 2 };

struct FacePose {
    GravityAxis axis;     // accelerometer axis that reads ~+/-9.8 m/s^2 at rest
    int8_t      sign;     // +1 or -1
    uint8_t     rotation; // TFT setRotation() value that makes this face upright
};

// ============================================================================
// FILL IN FROM BRING-UP (TSK-413 raw accel capture -> TSK-453)
//
// The cube rolls around its screen normal, so all four resting positions
// put gravity in the SAME plane: every row below should use the same two
// in-plane axes (e.g. +X, +Y, -X, -Y) with signs and rotations read off the
// real cube. The rows below reproduce the pre-bring-up guess, which spreads
// the faces over Z, X and Y. On hardware one of these faces is unreachable
// and one real resting position is ignored. Do not "fix" them without the
// raw x/y/z readings for each position.
// ============================================================================
static const FacePose FACE_POSES[] = {
    { AXIS_Z, +1, 0 },  // Face 1: Clock      (guess)
    { AXIS_X, +1, 1 },  // Face 2: Pomodoro   (guess)
    { AXIS_Y, +1, 2 },  // Face 3: Mascot     (guess)
    { AXIS_X, -1, 3 },  // Face 4: Ambient    (guess)
};
static const int FACE_COUNT = sizeof(FACE_POSES) / sizeof(FACE_POSES[0]);

// Pose id 0..5 (axis * 2 + negative) of the dominant gravity axis.
// Ties resolve Z, then X, then Y (unchanged from the original if-chain).
inline int classifyPose(float x, float y, float z) {
    float ax = fabsf(x), ay = fabsf(y), az = fabsf(z);
    if (az >= ax && az >= ay) return AXIS_Z * 2 + (z > 0 ? 0 : 1);
    if (ax >= ay && ax >= az) return AXIS_X * 2 + (x > 0 ? 0 : 1);
    return AXIS_Y * 2 + (y > 0 ? 0 : 1);
}

// Face index for a pose id, or -1 if that position is not a face.
inline int faceForPose(int poseId) {
    for (int f = 0; f < FACE_COUNT; f++) {
        if (FACE_POSES[f].axis * 2 + (FACE_POSES[f].sign > 0 ? 0 : 1) == poseId) return f;
    }
    return -1;
}

// ============================================================================
// FILL IN FROM BRING-UP: the screen's outward normal in accelerometer axes.
// The cube rolls around it, so it is the same for every face. If left/right
// steering in the maze comes out mirrored on the real cube, flip the sign.
// ============================================================================
static const GravityAxis SCREEN_OUT_AXIS = AXIS_Z;  // guess
static const int8_t      SCREEN_OUT_SIGN = +1;      // guess

struct KVec3 { float x, y, z; };

inline KVec3 kAxis(GravityAxis a, float s) {
    KVec3 v = { 0, 0, 0 };
    if (a == AXIS_X) v.x = s; else if (a == AXIS_Y) v.y = s; else v.z = s;
    return v;
}
inline float kDot(const KVec3& a, const KVec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline KVec3 kCross(const KVec3& a, const KVec3& b) {
    KVec3 v = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    return v;
}

// Screen frame of a face in accelerometer axes (unit vectors). up is what the
// accelerometer reads at rest (it measures the reaction to gravity). right and
// out complete a right-handed frame: right = up x out.
inline void screenFrame(int face, KVec3& up, KVec3& right, KVec3& out) {
    if (face < 0 || face >= FACE_COUNT) face = 0;
    up = kAxis(FACE_POSES[face].axis, FACE_POSES[face].sign);
    out = kAxis(SCREEN_OUT_AXIS, SCREEN_OUT_SIGN);
    right = kCross(up, out);
    if (kDot(right, right) < 0.5f) {
        // Guess table puts this face's gravity on the screen normal (see
        // above); pick any perpendicular axis so tilt still does something.
        out = kAxis(up.x != 0 ? AXIS_Y : AXIS_X, 1);
        right = kCross(up, out);
    }
}

// Accelerometer reading (m/s^2) for a face tilted by roll (+ = right side
// down) and pitch (+ = top edge away from the viewer), in degrees.
inline KVec3 tiltedGravity(int face, float rollDeg, float pitchDeg) {
    KVec3 up, right, out;
    screenFrame(face, up, right, out);
    const float d2r = 0.01745329f;
    float cr = cosf(rollDeg * d2r), sr = sinf(rollDeg * d2r);
    float cp = cosf(pitchDeg * d2r), sp = sinf(pitchDeg * d2r);
    // Right side down tips "up" towards -right; top away tips it towards +out
    KVec3 v = { up.x * cr * cp - right.x * sr + out.x * sp,
                up.y * cr * cp - right.y * sr + out.y * sp,
                up.z * cr * cp - right.z * sr + out.z * sp };
    float n = sqrtf(kDot(v, v));
    if (n > 0) { v.x *= 9.8f / n; v.y *= 9.8f / n; v.z *= 9.8f / n; }
    return v;
}

// Resting gravity vector for a face (used by the simulator's face buttons).
inline void gravityForFace(int face, float& x, float& y, float& z) {
    x = y = z = 0.0f;
    if (face < 0 || face >= FACE_COUNT) return;
    float g = 9.8f * FACE_POSES[face].sign;
    if (FACE_POSES[face].axis == AXIS_X) x = g;
    else if (FACE_POSES[face].axis == AXIS_Y) y = g;
    else z = g;
}
