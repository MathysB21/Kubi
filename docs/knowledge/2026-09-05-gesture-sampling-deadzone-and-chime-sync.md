# 2026-09-05: Gesture Sampling Deadzone Resolution & Reactive Chime Synchronization

## 1. Context & Motivation
During interactive testing with the desktop Hardware Simulation Studio, users reported that physical manipulation triggers (Gentle Tap, Shake) were "hit or miss"—sometimes requiring double clicks to register on screen, even though the 8-bit audio chime sounded immediately on the first click. Furthermore, clicking Tap on Face 3 (Mascot Face) produced an audio chime but no visual feedback whatsoever.

Investigation revealed two root causes:
1. A race condition between the 20ms polling interval in `SensorManager` and the HTTP injection endpoint.
2. Speculative client-side chime synthesis in the React frontend combined with contextual gesture handling in firmware.

---

## 2. Key Architecture & Design Decisions

### 1. The 20ms Accelerometer Polling Race Condition (Firmware Simulator)
In physical silicon and in simulation, [`SensorManager::loop()`](file:///c:/Development/Kubi/firmware/src/SensorManager.cpp) polls the ADXL345 accelerometer at ~50Hz with a debounce guard:
```cpp
if (now - _lastPollTime < 20) return; // Only samples every 20ms
```
Previously, the simulator's HTTP injection handler (`POST /sim/inject`) simulated a tap by temporarily incrementing the acceleration variable, calling `sensors.loop()`, and immediately restoring the baseline vector. If an HTTP request arrived during the 20ms sleep window, `sensors.loop()` returned immediately without reading the spike, and the vector reset before the next hardware tick. The gesture fell into a literal 20ms sampling deadzone.

### 2. Atomic Gesture Latching
The transient vector spike was replaced with an atomic gesture queue in [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp):
```cpp
static std::atomic<KubiGesture> sim_injected_gesture(GESTURE_NONE);
```
- In `POST /sim/inject`, incoming gestures (`"tap"`, `"shake"`, `"slam"`) are stored in `sim_injected_gesture`.
- In `hardwareSimulationThread()`, the gesture is latched every 10ms via:
  ```cpp
  KubiGesture injected = sim_injected_gesture.exchange(GESTURE_NONE);
  if (injected != GESTURE_NONE) gesture = injected;
  ```
- The HTTP handler pauses for 15ms (`std::this_thread::sleep_for(std::chrono::milliseconds(15))`) to allow the simulation loop to consume the gesture and invoke the appropriate firmware routines.
- This guarantees 100% capture rate without dropping clicks.

### 3. Contextual Gesture Behavior Across Faces
In the Kubi firmware, physical gestures have distinct contextual meanings depending on the active screen:
- **Face 1 (Focus Clock)**:
  - **Tap**: Toggles expanded calendar and financial quote details (`clockShowDetails`), auto-collapsing after 10s. Plays `CHIME_TAP_FEEDBACK`.
  - **Shake**: *Ignored in firmware*.
- **Face 2 (Pomodoro Timer)**:
  - **Tap**: Pauses or resumes the countdown timer (or dismisses an active chime if ringing). Plays `CHIME_TAP_FEEDBACK`.
  - **Shake**: Skips the current Pomodoro phase (Work $\leftrightarrow$ Short Break $\leftrightarrow$ Long Break). Plays `CHIME_POMODORO_DONE`.
- **Face 3 (Mascot Routine)**:
  - **Tap**: *Ignored in firmware* (mascot expressions and animations react purely to time of day and room temperature).
  - **Shake**: *Ignored in firmware*.
- **Face 4 (Schedule Agenda)**:
  - **Tap**: Cycles between page 0 and page 1 of cached agenda items. Plays `CHIME_TAP_FEEDBACK`.
  - **Shake**: *Ignored in firmware*.
- **Desk Slam (`GESTURE_SLAM`)**:
  - *Universal across all faces*: Plays `CHIME_SLAM_OUCH` retro error buzzer.

### 4. Reactive Chime Synchronization
Previously, the frontend played `CHIME_TAP_FEEDBACK` immediately upon button click before sending the HTTP request. This created a "phantom sound"—users heard a chime on Face 3 even though the firmware performed no action.
- The speculative client-side chime calls were removed from `triggerGesture()` in [`dashboard/src/pages/Simulator.tsx`](file:///c:/Development/Kubi/dashboard/src/pages/Simulator.tsx).
- `POST /sim/inject` returns `{ lastChime, lastChimeTime }` after the hardware thread processes the event.
- The browser Web Audio synthesizer only plays a chime when the C++ firmware actually triggers `audio.playChime(...)`.

---

## 3. Code Touchpoints & Files
- [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Atomic gesture queue (`sim_injected_gesture`), gesture exchange in hardware loop, and `POST /sim/inject` response payload.
- [`firmware/src/SensorManager.cpp`](file:///c:/Development/Kubi/firmware/src/SensorManager.cpp): Real ADXL345 20ms sampling window and transient gesture detection.
- [`firmware/src/main.cpp`](file:///c:/Development/Kubi/firmware/src/main.cpp): Production firmware gesture state machine.
- [`dashboard/src/pages/Simulator.tsx`](file:///c:/Development/Kubi/dashboard/src/pages/Simulator.tsx): Client injection helper, reactive chime playback, and elimination of speculative sound cues.

---

## 4. Gotchas & Verification

> [!WARNING]
> Never simulate physical button or accelerometer taps using ephemeral in-memory variable spikes without thread synchronization or queue latching. A background thread running with `sleep_for(10ms)` or `sleep_for(20ms)` will periodically miss unlatched transient states.

### Verification Steps
1. Navigate to Face 1 (Focus Clock): Click "Gentle Tap" $\rightarrow$ Screen toggles details card with a crisp audio pip.
2. Navigate to Face 2 (Pomodoro): Click "Gentle Tap" $\rightarrow$ Timer toggles between counting and white centered "PAUSED" text with audio pip.
3. Click "Shake Cube" on Face 2 $\rightarrow$ Pomodoro advances to next cycle/break with completion chime.
4. Navigate to Face 3 (Mascot): Click "Gentle Tap" $\rightarrow$ Screen does not change and **no sound is played** (correct firmware behavior).
5. Click "Desk Slam!" on any face $\rightarrow$ Plays "Ouch!" retro chime.
