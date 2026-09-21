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

// Resting gravity vector for a face (used by the simulator's face buttons).
inline void gravityForFace(int face, float& x, float& y, float& z) {
    x = y = z = 0.0f;
    if (face < 0 || face >= FACE_COUNT) return;
    float g = 9.8f * FACE_POSES[face].sign;
    if (FACE_POSES[face].axis == AXIS_X) x = g;
    else if (FACE_POSES[face].axis == AXIS_Y) y = g;
    else z = g;
}
