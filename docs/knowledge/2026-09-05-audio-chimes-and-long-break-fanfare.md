# 2026-09-05: Audio Chime System and Long Break Celebratory Fanfare

## 1. Context & Motivation
Kubi uses audio feedback to signal Pomodoro phase transitions, user gestures, and boot states.
- The standard Pomodoro completion chime (`CHIME_POMODORO_DONE`) is a 4-note ascending C-major arpeggio (~550ms) used for standard focus session completion into short breaks.
- Completing a full cycle of Pomodoro sessions into the Long Break is a major achievement that warrants a special, extended celebratory fanfare.

## 2. Key Architecture & Design Decisions

### Chime Palette & Long Break Fanfare
- **Enum**: `CHIME_POMODORO_LONG_BREAK` added to [`AudioManager.h`](file:///c:/Development/Kubi/firmware/src/AudioManager.h).
- **Melody Structure**: Extended 7-note triumphant fanfare:
  - C5 (523 Hz, 90ms) $\to$ E5 (659 Hz, 90ms) $\to$ G5 (784 Hz, 90ms) $\to$ C6 (1046 Hz, 130ms) $\to$ D6 (1175 Hz, 100ms) $\to$ E6 (1318 Hz, 120ms) $\to$ G6 (1568 Hz, 350ms). Total duration: ~970ms.
- **Trigger**: Played in `PomodoroManager::tick()` and `skipCurrent()` whenever transitioning into `POMODORO_LONG_BREAK`.

### Simulator Audio Telemetry Synchronization
- Added `audio.getActiveChime()` in `AudioManager`.
- In `sim_main.cpp` background thread, if a chime starts playing in the C++ firmware, it is immediately synchronized into `sim_state.lastChime` and streamed via `/sim/state`.
- Web Audio synthesizer in [`dashboard/src/lib/audioChimes.ts`](file:///c:/Development/Kubi/dashboard/src/lib/audioChimes.ts) polls `/sim/state` and synthesizes the exact square-wave 8-bit frequencies in real time in the browser.

## 3. Code Touchpoints & Files
- [`firmware/src/AudioManager.h`](file:///c:/Development/Kubi/firmware/src/AudioManager.h): Declared `CHIME_POMODORO_LONG_BREAK` and `getActiveChime()`.
- [`firmware/src/AudioManager.cpp`](file:///c:/Development/Kubi/firmware/src/AudioManager.cpp): Implemented frequency/duration sequence and tracking state.
- [`firmware/src/PomodoroManager.cpp`](file:///c:/Development/Kubi/firmware/src/PomodoroManager.cpp): Dispatched `CHIME_POMODORO_LONG_BREAK` when entering long break.
- [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Synchronized active chime to HTTP telemetry.
- [`dashboard/src/lib/audioChimes.ts`](file:///c:/Development/Kubi/dashboard/src/lib/audioChimes.ts): Added Web Audio frequency tables for the 7-note melody.

## 4. Gotchas & Verification
- **Gotcha**: The simulator does not output real I2S audio hardware signals; it relies on the dashboard's Web Audio synthesizer reading `lastChime` from `/sim/state`.
- **Verification**: In dashboard or simulator, trigger a skip or fast-forward to the end of session 4. Verify the 7-note fanfare plays cleanly and transitions into `LONG BREAK`.
