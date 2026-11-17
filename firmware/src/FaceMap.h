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
// MEASURED ON HARDWARE IN THE WOODEN CUBE (2026-11-17, 20-sample averages of
// /api/diagnostics, the same procedure as firmware/tools/capture_faces.ps1)
//
// The cube rolls around its screen normal, so all four resting positions put
// gravity in the same plane: two axes, in opposite pairs. Face 1 is the panel
// upright in portrait; each next face is a quarter turn to the left (top edge
// goes left). Raw readings in m/s^2; Z reads ~-1.5 at rest on every face,
// which is this ADXL345's zero-g offset, not tilt.
//
// The breadboard calibration (2026-10-07) had Face 1 on +X; in the cube the
// IMU lies flat on the perfboard facing the back plate, so the axes moved.
// If the sensor ever moves relative to the screen again, rerun the script and
// paste its output here. Rotations assume a left roll means setRotation()+1;
// if Faces 2 and 4 render upside down, swap their rotations (1 <-> 3).
// ============================================================================
static const FacePose FACE_POSES[] = {
    { AXIS_Y, +1, 0 },  // Face 1: Clock     measured (  0.1,  10.2, -1.8)
    { AXIS_X, -1, 1 },  // Face 2: Pomodoro  measured (-10.5,   0.3, -1.4)
    { AXIS_Y, -1, 2 },  // Face 3: Mascot    measured ( -0.7, -10.5, -1.1)
    { AXIS_X, +1, 3 },  // Face 4: Ambient   measured ( 10.0,  -0.5, -1.5)
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

// Lying on its back, screen to the sky: the tilt maze (decided 2026-11-17).
// Not a fifth roll position: it is off the roll plane, on the screen normal.
// Screen down stays "no face" (the cube keeps whatever it last showed).
static const int FACE_SCREEN_UP = FACE_COUNT;

// Face index for a pose id: 0..FACE_COUNT-1, FACE_SCREEN_UP, or -1 if that
// position is not a face. (SCREEN_OUT_* is defined below.)
inline int screenUpPose();
inline int faceForPose(int poseId) {
    for (int f = 0; f < FACE_COUNT; f++) {
        if (FACE_POSES[f].axis * 2 + (FACE_POSES[f].sign > 0 ? 0 : 1) == poseId) return f;
    }
    if (poseId == screenUpPose()) return FACE_SCREEN_UP;
    return -1;
}

// ============================================================================
// The screen's outward normal in accelerometer axes (towards the viewer),
// derived from Face 1 and Face 2 above: up(Face 2) x up(Face 1) = -X x +Y.
// It was +Z on the breadboard (sensor facing the viewer); in the cube the IMU
// faces the back plate, so it is -Z. If left/right steering in the maze comes
// out mirrored on the real cube, flip the sign.
// ============================================================================
static const GravityAxis SCREEN_OUT_AXIS = AXIS_Z;
static const int8_t      SCREEN_OUT_SIGN = -1;

// Pose id of the cube lying on its back (the accelerometer reads +out).
inline int screenUpPose() { return SCREEN_OUT_AXIS * 2 + (SCREEN_OUT_SIGN > 0 ? 0 : 1); }

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
        // Only if a table row ever puts gravity on the screen normal (a bad
        // capture); pick any perpendicular axis so tilt still does something.
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

// Lying screen-up, the maze is drawn the way viewFace (the upright face it was
// tipped back from) draws, so it reads right from where its player sits. Its
// screen axes are body-fixed, so they are viewFace's: up is the screen's top
// edge, right its right edge, and the accelerometer reads +out at rest.
// Reading for that pose tilted by roll (+ = right side down) and pitch
// (+ = top edge down), in degrees. Used by the simulator's tilt pad.
inline KVec3 tiltedGravityFlat(int viewFace, float rollDeg, float pitchDeg) {
    KVec3 up, right, out;
    screenFrame(viewFace, up, right, out);
    const float d2r = 0.01745329f;
    float cr = cosf(rollDeg * d2r), sr = sinf(rollDeg * d2r);
    float cp = cosf(pitchDeg * d2r), sp = sinf(pitchDeg * d2r);
    KVec3 v = { out.x * cr * cp - right.x * sr - up.x * sp,
                out.y * cr * cp - right.y * sr - up.y * sp,
                out.z * cr * cp - right.z * sr - up.z * sp };
    float n = sqrtf(kDot(v, v));
    if (n > 0) { v.x *= 9.8f / n; v.y *= 9.8f / n; v.z *= 9.8f / n; }
    return v;
}

// Resting gravity vector for a face (used by the simulator's face buttons).
// FACE_SCREEN_UP is the cube on its back.
inline void gravityForFace(int face, float& x, float& y, float& z) {
    x = y = z = 0.0f;
    if (face == FACE_SCREEN_UP) {
        KVec3 o = kAxis(SCREEN_OUT_AXIS, SCREEN_OUT_SIGN);
        x = 9.8f * o.x; y = 9.8f * o.y; z = 9.8f * o.z;
        return;
    }
    if (face < 0 || face >= FACE_COUNT) return;
    float g = 9.8f * FACE_POSES[face].sign;
    if (FACE_POSES[face].axis == AXIS_X) x = g;
    else if (FACE_POSES[face].axis == AXIS_Y) y = g;
    else z = g;
}
