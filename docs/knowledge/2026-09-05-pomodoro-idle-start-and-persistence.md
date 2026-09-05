# 2026-09-05: Pomodoro Timer Idle State, Minimal UI, and Face Persistence

## 1. Context & Motivation
The Pomodoro face (Face 2) previously had several user experience issues:
1. **Auto-Countdown on Boot/Reset**: The timer immediately started ticking down on load without user confirmation.
2. **Text Clipping & Clutter**: Paused state showed long red text `"PAUSED (Tap: Play | Shake: Skip)"`, which overflowed the 320px landscape display width and clipped.
3. **Unwanted Resumption**: When paused, navigating away to another face and returning resumed the countdown automatically.

## 2. Key Architecture & Design Decisions

### Idle / Not-Started Lifecycle State
- Added `bool _isStarted` to `PomodoroManager`.
- Initial state on boot and after `resetCurrent()` is `_isStarted = false`.
- `tick()` only decrements time if `_isStarted && !_isPaused`.
- Displays retro arcade-style flashing `"START"` text in white (~1.2s period: 600ms visible, 600ms hidden).
- Triggering play via gentle tap, screen click, or companion dashboard sets `_isStarted = true`.

### Clean Minimal Display
- Removed verbose interaction instructions; user knows how to interact with Kubi.
- When paused and `_isStarted == true`, displays clean centered white `"PAUSED"` text above the progress bar.
- Replaced red text with clean crisp white to match the theme.

### Pause State Persistence Across Faces
- Removed unconditional `pomodoro.play()` calls on face transitions in both `main.cpp` and `sim_main.cpp`.
- If the user leaves Face 2 while paused, returning to Face 2 preserves the paused state.

## 3. Code Touchpoints & Files
- [`firmware/src/PomodoroManager.h`](file:///c:/Development/Kubi/firmware/src/PomodoroManager.h): Added `_isStarted` member and `isStarted()` getter.
- [`firmware/src/PomodoroManager.cpp`](file:///c:/Development/Kubi/firmware/src/PomodoroManager.cpp): Guarded `tick()` with `_isStarted`; updated `play()`, `resetCurrent()`.
- [`firmware/src/DisplayManager.cpp`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp): In `drawPomodoro()`, implemented slow-flash `"START"` when `!isStarted` and minimal white `"PAUSED"` when paused.
- [`firmware/src/main.cpp`](file:///c:/Development/Kubi/firmware/src/main.cpp) & [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Drained gestures on face switch and eliminated auto-play on face switch.
- [`dashboard/src/App.tsx`](file:///c:/Development/Kubi/dashboard/src/App.tsx): Dynamic Start/Pause/Resume button based on `isStarted` and `isPaused`.

## 4. Gotchas & Verification
- **Gotcha**: Always drain gestures when switching faces so the physical turn doesn't register as a tap and immediately start/pause the timer.
- **Verification**: In simulator or hardware, switch to Face 2. Observe flashing `"START"`. Tap to start countdown. Tap to pause ("PAUSED"). Rotate to Face 1 (Clock) then back to Face 2. Verify it remains in "PAUSED" state.
