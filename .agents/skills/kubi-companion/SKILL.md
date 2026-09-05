---
name: kubi-companion
description: >-
  Comprehensive guide and runbook for developing, simulating, testing, and debugging
  Project Kubi (firmware, desktop SITL simulator, and companion web dashboard).
---

# Project Kubi Development & Companion Guide

This skill provides step-by-step instructions, runbooks, architectural constraints, and operational guidelines for working on **Project Kubi** — a smart, haptic ESP32 companion cube with an ST7789 IPS screen, ADXL345 accelerometer, BMP280 environmental sensor, and I2S audio chimes.

---

## 1. Quick Verification Runbook

Always verify all three build pipelines before concluding any work:

### 1.1 Real ESP32 Silicon Firmware Build (PlatformIO)
Must build cleanly with zero errors:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```
Build output: `firmware/.pio/build/esp32dev/firmware.bin`.

### 1.2 Frontend Companion Dashboard Build (Vite / React / TypeScript)
Must build cleanly and update `firmware/data/`:
```powershell
cd dashboard
npm run build
```

### 1.3 Desktop Simulator Verification (LLVM-MinGW clang++)
Verify compilation passes without locking or executing:
```powershell
$firmwareDir = "c:\Development\Kubi\firmware"
$scriptDir = "c:\Development\Kubi\firmware\sim"
$wingetClang = (Get-ChildItem "C:\Users\Mathys\AppData\Local\Microsoft\WinGet\Packages" -Recurse -Filter "clang++.exe" -ErrorAction SilentlyContinue | Select-Object -First 1).FullName
$srcFiles = @(
    "$firmwareDir\src\PomodoroManager.cpp",
    "$firmwareDir\src\DisplayManager.cpp",
    "$firmwareDir\src\SensorManager.cpp",
    "$firmwareDir\src\AudioManager.cpp",
    "$firmwareDir\src\KubiSprites.cpp",
    "$scriptDir\src\TFT_eSPI_Mock.cpp",
    "$scriptDir\src\sim_main.cpp"
)
$includes = @("-I", "$scriptDir\include", "-I", "$firmwareDir\include", "-I", "$firmwareDir\src", "-I", "$firmwareDir\.pio\libdeps\esp32dev\ArduinoJson\src")
$outputTest = "$scriptDir\kubi_sim_test.exe"
& $wingetClang -std=c++17 -O2 -static $includes $srcFiles -lws2_32 -o $outputTest
if ($LASTEXITCODE -eq 0) { Remove-Item -Force $outputTest; Write-Host "SIM_BUILD_OK" }
```

---

## 2. Core Operational Rules for Agents

1. **NEVER Launch or Kill `kubi_sim.exe` Automatically**:
   - The user runs `kubi_sim.exe` in an external terminal window.
   - Do NOT kill `kubi_sim.exe` or invoke `run_sim.ps1`.
   - Windows locks active binaries: compiling to `kubi_sim.exe` while running fails with permission errors. Always compile tests to a temporary binary (e.g. `kubi_sim_test.exe`).
2. **Zero Code Duplication**:
   - `firmware/src/` sources (`DisplayManager.cpp`, `PomodoroManager.cpp`, `SensorManager.cpp`, `AudioManager.cpp`, `KubiSprites.cpp`) are shared 1:1 between real silicon and desktop simulation.
   - Keep simulation mocks isolated in `firmware/sim/include/` and `firmware/sim/src/`.
3. **Orientation Changes & False Tap Suppression**:
   - Physical flips produce high transient accelerations (`deltaMag > 7.0f`).
   - In `SensorManager::updateFace()`, always update `_lastTapTime = millis()` and set `_recentGesture = GESTURE_NONE` during candidate face transitions and settling.
   - Always flush gestures (`sensors.getRecentGesture()`) upon face transitions in `main.cpp` and `sim_main.cpp`.
4. **C/C++ Preprocessor Comment Rule**:
   - Never end single-line comments with trailing backslashes (`// \`), as C++ treats them as line continuations.

---

## 3. Face Modes & Functional Specifications

* **Face 1: Focus Clock Face**:
  - Minimal digital 7-segment time display with date (`[Day, D Mon]`).
  - Shake gesture (`GESTURE_SHAKE`) toggles between digital 7-segment clock and clean analog clock view (radial numbers, orange hour hand, white minute hand, center pivot, no circular border).
  - Mode choice is persisted to NVS Preferences (`"clockAnalog"`).
* **Face 2: Pomodoro Timer**:
  - **Not Started / Idle State**: If `!isStarted`, timer does not decrement. Displays slow retro arcade-style flashing `"START"` text in white (~1.2s period: 600ms on, 600ms off).
  - **Running State**: Clean countdown, progress bar at bottom, no cluttering interaction instruction text.
  - **Paused State**: Centered white `"PAUSED"` text.
  - **Pause State Persistence**: Navigating away from Face 2 and returning preserves the paused state without auto-resuming. Never unconditionally call `pomodoro.play()` on face transitions.
  - **Reset Action**: Sets `_isStarted = false`, `_isPaused = false`, restores full duration, and displays flashing `"START"`.
* **Face 3: Mascot Routine & Room Temp**:
  - Animated bouncing Kubi jelly character and live room temperature readings from BMP280.
* **Face 4: Schedule Agenda**:
  - Google Calendar 3-day agenda fetched via iCal URL. Gentle tap cycles agenda pages.

---

## 4. Chime Audio Palette & Sound Engine

All chimes are synthesized via I2S (`MAX98357A`) on hardware and Web Audio square wave in simulator:

1. `CHIME_TAP_FEEDBACK`: Single 880 Hz ping (35ms).
2. `CHIME_WAKE_PING`: 2-note ascending ping (D5 587Hz -> A5 880Hz).
3. `CHIME_POMODORO_DONE`: 4-note ascending C major arpeggio (~550ms):
   - C5 (523Hz, 100ms) -> E5 (659Hz, 100ms) -> G5 (784Hz, 100ms) -> C6 (1046Hz, 250ms).
   - Played when a focus session completes into a Short Break.
4. `CHIME_POMODORO_LONG_BREAK`: Unique 7-note celebratory fanfare (~970ms):
   - C5 (523Hz, 90ms) -> E5 (659Hz, 90ms) -> G5 (784Hz, 90ms) -> C6 (1046Hz, 130ms) -> D6 (1175Hz, 100ms) -> E6 (1318Hz, 120ms) -> G6 (1568Hz, 350ms).
   - Played when all cycles finish and transitioning into the Long Break.
5. `CHIME_SLAM_OUCH`: 3-note descending buzzer (F4 349Hz -> C4 262Hz -> F3 175Hz).
