# Kubi Project & Agent Instructions (AGENTS.md)

Welcome! This repository contains the complete firmware, companion web dashboard, and desktop hardware simulation environment for **Project Kubi** — a smart, haptic ESP32 desk companion cube featuring a 240×320 ST7789 IPS display, ADXL345 accelerometer for orientation & gestures, BMP280 environmental sensor, and I2S audio chimes.

---

## 1. Quick Reference: Kubi Hardware Simulator (Digital Twin)

> [!IMPORTANT]
> **A Software-in-the-Loop (SITL) Digital Twin exists in this codebase!**
> If the user mentions "the sim", "digital twin", or shares screenshots of a wooden companion cube with brass screws and a screen, they are working in the **Kubi Hardware Simulation Studio**.

### How to Launch the Simulator
From the repository root or `dashboard/`:
```bash
cd dashboard
npm run sim
```
Or via PowerShell directly:
```powershell
powershell -ExecutionPolicy Bypass -File firmware/sim/run_sim.ps1
```

### URLs & Ports
* **Workbench UI**: `http://localhost:5173/sim`
* **C++ Firmware Runner HTTP API**: `http://127.0.0.1:8080`
  - `GET /sim/frame`: Streams raw 240×320 RGBA pixel buffer from the C++ framebuffer.
  - `POST /sim/inject`: Injects virtual face changes (`{"face": 1}`), gestures (`{"gesture": "tap"|"shake"|"slam"}`), temp, battery, accel.
  - `GET /sim/state`: Telemetry heartbeat (orientation, backlight PWM, chime events, active face).
  - `GET /api/state`: Standard Kubi firmware REST state.
  - `POST /api/settings`: Updates Pomodoro intervals, colors, etc.
  - `POST /api/override`: Triggers the companion "Secret Override" broadcast message.

### How to Rebuild the Simulator
```powershell
powershell -ExecutionPolicy Bypass -File firmware/sim/build_sim.ps1
```
Uses the installed LLVM-MinGW `clang++` toolchain (`-std=c++17 -O2 -static`).

---

## 2. Recognizing Simulator Screenshots

When the user shares a screenshot containing:
1. **A Wooden Cube Chassis with 4 Brass Machine Screws**:
   - Represents the physical 10cm companion cube on the user's desk.
   - Screen inside is a 240×320 ST7789 IPS panel with realistic glass glare reflection.
   - **Auto-Orient Mode**: When Face 2 (Pomodoro Timer) or Face 4 is selected, the cube chassis physically and smoothly rolls $-90^\circ$ onto its side into widescreen landscape ($320\times 240$) right-side up.
   - Clicking the screen triggers a simulated gentle tap.
2. **Right-Hand Panel**:
   - **Hardware Rig Tab**: Orientation buttons (Face 1: Clock, Face 2: Pomodoro, Face 3: Mascot, Face 4: Schedule), Gesture triggers (Gentle Tap, Shake to skip, Desk Slam ouch!), sliders for Temperature, Time Machine, Battery, and 3-axis Accelerometer.
   - **Side-by-Side Dashboard Tab**: Embeds the full live companion dashboard.
3. **8-Bit Web Audio**:
   - Real-time synthesizer plays retro square-wave chimes whenever firmware triggers `audio.playChime(...)`.

---

## 3. Architecture: Zero Code Duplication

Kubi uses **Software-in-the-Loop** simulation:
* **Firmware Sources are Shared 1:1**:
  - `firmware/src/PomodoroManager.cpp`
  - `firmware/src/DisplayManager.cpp`
  - `firmware/src/SensorManager.cpp`
  - `firmware/src/AudioManager.cpp`
  - `firmware/src/KubiSprites.cpp`
* **Desktop HAL (`firmware/sim/include/`)**:
  - Replaces ESP32-specific peripherals (SPI, I2C, FreeRTOS tasks) with desktop equivalents without altering production firmware code.
  - `TFT_eSPI_Mock.cpp` rasterizes graphics, 5×7 standard ASCII fonts, and 7-segment timer digits into a memory buffer.
* **Production Silicon Build**:
  - PlatformIO builds the real hardware binary (`firmware.bin`) for ESP32.

---

## 4. Golden Rules for Future Agents

1. **Never Break the Real Hardware Build**:
   - Always verify that PlatformIO builds cleanly before and after changes:
     ```powershell
     & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
     ```
2. **Do NOT Launch or Kill the Simulator Automatically**:
   - The user runs `kubi_sim.exe` in an external shell window.
   - Do **NOT** invoke `run_sim.ps1` or run `kubi_sim.exe` in the background unless explicitly requested.
   - Because Windows locks the running `kubi_sim.exe` binary, attempting to compile directly to `kubi_sim.exe` with `build_sim.ps1` will fail with permission errors while the process is active. When testing clang++ builds, output to a temporary binary (e.g., `kubi_sim_test.exe`) or ask the user to restart/rebuild.
3. **Keep Core Firmware Pure**:
   - Do not add desktop-specific `#ifdef` hacks directly into `firmware/src/*.cpp` unless strictly necessary. Keep simulation mocks isolated inside `firmware/sim/`.
4. **Suppress False Taps During Orientation Changes**:
   - Rolling or flipping the cube produces high accelerometer delta transients (`deltaMag > 7.0f`).
   - In `SensorManager::updateFace()`, always update `_lastTapTime = millis()` and reset `_recentGesture = GESTURE_NONE` during candidate face transitions and when orientation settles to avoid triggering false tap gestures. Drain any pending gestures when switching faces.
5. **C/C++ Preprocessor Comment Gotcha**:
   - Never end a single-line comment with a trailing backslash (`// \`). In standard C/C++, this joins the next line to the comment as a line continuation, deleting whatever was on that line!
6. **When Modifying Frontend**:
   - Always run `npm run build` in `dashboard/` to verify TypeScript types and Vite build integrity.

---

## 5. Face Modes & Functional Specifications

* **Face 1: Focus Clock Face**:
  - Clean minimalist digital time display (large 7-segment font) centered on screen.
  - Date display formatted as `[Day, D Mon]` (e.g. `Thu, 3 Sep`, `Sat, 8 Aug`) centered 40px below the time in 1/4 size font (Font 2).
  - **Shake Gesture (`GESTURE_SHAKE`)**: Toggles between digital 7-segment clock and a clean analog clock view (12 radial numbers, 3px orange hour hand, 2px white minute hand, center pivot, and date below, without any circular border).
  - **View Persistence**: The user's digital vs. analog view choice is preserved in NVS Preferences (`"kubi_settings"` namespace, key `"clockAnalog"`) on hardware and in `kubi_sim_prefs.txt` in the simulator.
* **Face 2: Pomodoro Timer**:
  - Focus and break countdowns with cycle tracking and customized phase colors.
  - **Not Started / Idle State**: If the timer has not been started yet (or was reset), it does not count down; instead, it displays a slow retro arcade-style flashing `"START"` text (~1.2s period) in white. A gentle tap or dashboard Play starts the timer.
  - Gentle tap toggles pause/play. Shake gesture skips to next phase.
  - Minimal UI: Shows clean centered white "PAUSED" text when paused; no cluttering interaction instruction text.
  - **Pause State Persistence**: If the timer is paused, navigating to another face and returning preserves the paused state without auto-resuming.
  - **Chimes**:
    - `CHIME_POMODORO_DONE`: 4-note ascending C major arpeggio (C5 -> E5 -> G5 -> C6) for focus session completion and short breaks.
    - `CHIME_POMODORO_LONG_BREAK`: Unique extended 7-note triumphant fanfare (C5 -> E5 -> G5 -> C6 -> D6 -> E6 -> G6) played when all cycles finish and transitioning into the Long Break.
* **Face 3: Mascot & Room Temp**:
  - Animated bouncing Kubi jelly character and live room temperature readings from BMP280.
* **Face 4: Schedule Agenda**:
  - Google Calendar 3-day agenda fetched via iCal URL. Gentle tap pages through agenda items.

---

## 6. Desktop Simulation HAL & Preferences Persistence

- Desktop mocks reside in `firmware/sim/include/`.
- `Preferences.h` mock replicates the ESP32 NVS `Preferences` API and synchronizes key-value pairs to `kubi_sim_prefs.txt` in the working directory on `put*()` / `end()`. This ensures settings such as `clockAnalog`, Pomodoro intervals, and iCal URLs survive simulator restarts without hardware connected.

---

## 7. Knowledge Base & Session Documentation (`docs/knowledge/`)

- Project knowledge documents reside in `docs/knowledge/`.
- **Skill**: Activate the `project-knowledge` skill (`.agents/skills/project-knowledge/SKILL.md`).
- **Reading Rule**: When consulting past decisions or features, read **ONLY** `docs/knowledge/index.md` first. Inspect the index table, select only the relevant topic doc(s), and do **not** dump all documents into context.
- **Recording Rule**: When asked to record session knowledge, create one or more dated docs (`YYYY-MM-DD-<slug>.md`) in `docs/knowledge/` separating topics cleanly, update `docs/knowledge/index.md`, and commit.

