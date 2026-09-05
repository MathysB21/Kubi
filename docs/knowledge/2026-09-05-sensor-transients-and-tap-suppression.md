# 2026-09-05: Sensor Transients and False Tap Suppression

## 1. Context & Motivation
Kubi relies on an ADXL345 accelerometer for orientation detection and gesture recognition (Gentle Tap, Shake, Desk Slam).
- Physical rotation or flipping of the cube produces angular acceleration and impulse spikes ($\Delta\text{Mag} > 7.0\text{ m/s}^2$).
- Without suppression, rotating the cube to Face 2 (Pomodoro) would falsely register as a tap, immediately toggling pause/play or triggering unwanted actions.

## 2. Key Architecture & Design Decisions

### Orientation Debounce & Transient Suppression
- In [`SensorManager::updateFace()`](file:///c:/Development/Kubi/firmware/src/SensorManager.cpp):
  1. **On Candidate Change**: Whenever gravity readings indicate a transition to a new face candidate, update `_lastTapTime = millis()` and clear `_recentGesture = GESTURE_NONE`.
  2. **On Orientation Settle**: After candidate orientation remains stable for the debounce window (400ms), update `_lastTapTime = millis()` again and force `_recentGesture = GESTURE_NONE`.

### Face Transition Gesture Queue Draining
- In `main.cpp` and `sim_main.cpp`:
  - Whenever `activeFace != previousFace`, call `sensors.getRecentGesture()` to drain and discard any lingering gesture events before processing the new face logic.

## 3. Code Touchpoints & Files
- [`firmware/src/SensorManager.cpp`](file:///c:/Development/Kubi/firmware/src/SensorManager.cpp): Added tap suppression resets in `updateFace()`.
- [`firmware/src/main.cpp`](file:///c:/Development/Kubi/firmware/src/main.cpp): Drained lingering gestures on face switches in Core 1 loop.
- [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Mirror behavior for simulation loop.

## 4. Gotchas & Verification
- **Gotcha**: Do not decrease the settling debounce below 300ms, as the physical deceleration when the cube lands on its new face can still spike accelerometer deltas.
- **Verification**: Rotate cube back and forth between faces in the simulator or on physical hardware. Ensure no phantom taps occur during or immediately after the roll.
