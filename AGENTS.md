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
  - `POST /sim/inject`: Injects virtual face changes (`{"face": 1}`), gestures (`{"gesture": "tap"|"shake"|"slam"}`), continuous tilt (`{"tiltRoll": deg, "tiltPitch": deg}`, relative to the current face resting flat), the first-connect screen (`{"showAddress": true}`), temp, battery, accel.
  - `GET /sim/state`: Telemetry heartbeat (orientation, backlight PWM, chime events, active face, tilt, `audioSamples`/`audioUnderruns`, `pixelWrites`, and `maze` when Face 4 is the maze).
  - `GET /api/state`: Standard Kubi firmware REST state (incl. `sleepTimeoutMin`, `isSleeping`, `face4Maze`).
  - `POST /api/settings`: Updates Pomodoro intervals/colours, clock style, `sleepTimeoutMin`, `face4Maze`, face.
  - `POST /api/override`: Triggers the companion "Secret Override" broadcast message.
  - `POST /api/factory-reset`: `{"confirm":"ERASE"}` wipes settings + WiFi and restarts into setup (the sim replays onboarding instead of rebooting).

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
   - **Hardware Rig Tab**: Orientation buttons (Face 1: Clock, Face 2: Pomodoro, Face 3: Mascot, Face 4: Ambient), Gesture triggers (Gentle Tap, Shake to skip, Desk Slam ouch!), sliders for Temperature, Time Machine, Battery, and 3-axis Accelerometer.
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
  - `firmware/src/AmbientFace.cpp`, `firmware/src/MazeGame.cpp`, `firmware/src/MazeBoards.cpp`, `firmware/src/FaceMap.h`
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
6. **Editing the Setup Portal Strings Does Not Trigger a Rebuild**:
   - `lib/WiFiManager` pulls in `wm_strings_en.h` through `#include WM_STRINGS_FILE`, which the SCons dependency scanner cannot follow, and SCons compares content hashes, so `touch` does not help either. After editing that header, delete the library's objects before building (`Remove-Item -Recurse .pio\build\esp32dev\lib*\WiFiManager, .pio\build\esp32dev\lib*\libWiFiManager.a`) or run `pio run -t clean`. Confirm by searching `firmware.bin` for the new text.
7. **When Modifying Frontend**:
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
  - The only face (with the maze) that sleeps: after `sleepTimeoutMin` of stillness (default 5, NVS `sleepMin`, dashboard "Screen Sleep") the backlight fades out; any tap or movement wakes it silently.
* **Face 4: Ambient Glow** (`AmbientFace.cpp`, ships by default; the tilt maze may replace it):
  - Dithered pixel-art orb (8 px cells, 4x4 Bayer fringe) that breathes (9 s) and eases through a night palette (40 s per colour).
  - Backlight fades down to `AmbientFace::BACKLIGHT` on this face. Exempt from screen sleep. Tap moves on to the next colour, silently.
  - Redraws at 10 Hz and pushes only cells whose colour changed (~8k px/s).
* **Face 4 alternative: Tilt Maze** (`MazeGame.cpp`, `MazeBoards.cpp`; on when NVS `face4Maze` is set, dashboard "Face 4 shows: Maze (beta)"; go/no-go 2 Nov):
  - Ten 20x14 ASCII boards (`#` wall, `S`, `G`, `O` hole). Steering is tilt relative to the resting vector captured at board start, in FaceMap's screen frame. Fixed 200 Hz physics in cell units, circle-vs-cell collisions, no tunnelling at the speed cap.
  - While playing, faces need a 2 s hold to switch (roll over and hold to leave); the lock releases after 45 s without input.
  - Holes send the ball back to S (clock keeps running), the goal shows time + best (NVS `mzb<n>`), then the next board. Renders at 30 fps, ball only.

---

## 5b. Architecture Notes (read before changing these areas)

* **Audio** runs in its own FreeRTOS task (`audioTask` in `main.cpp`, a thread in the sim) that blocks on the I2S DMA queue. `playChime()` is safe from any task. The sim's `AudioOutputI2S` mock is deliberately strict: no samples before `begin()`, a DMA-sized queue drained at the sample rate, underruns counted.
* **Rendering** is scene-based (`DisplayManager::beginScene`). Only a scene change, rotation or `invalidate()` clears the screen; faces repaint just what changed. Never add `fillScreen` to a steady-state draw path. The TFT mock implements TFT_eSPI's opaque text background and `setTextPadding`, so overdraw bugs show in the sim.
* **Screen sleep** policy lives in `DisplayManager` (`noteActivity`, `setSleepAllowed`, `requestWake` for other tasks); which faces sleep is `modeMaySleep()` in `API.h`.
* **Orientation** has one source of truth: `FACE_POSES` and `SCREEN_OUT_*` in `FaceMap.h` (marked FILL IN FROM BRING-UP). Sensors, rotation, the sim's face buttons and tilt injection, and maze steering all read it.
* **Cross-task data**: never share an Arduino `String` between the web server task and the hardware loop; see `setOverrideText()/getOverrideText()`. Web handlers must not touch the TFT: they set state and the hardware loop applies it (e.g. rotation follows `currentMode`).
* **The orchestration is still duplicated**: every behaviour change in `main.cpp` `core1HardwareTask` / `API.cpp` must be mirrored in `sim_main.cpp`.

---

## 6. Desktop Simulation HAL & Preferences Persistence

- Desktop mocks reside in `firmware/sim/include/`.
- `Preferences.h` mock replicates the ESP32 NVS `Preferences` API and synchronizes key-value pairs to `kubi_sim_prefs.txt` in the working directory on `put*()` / `end()`. This ensures settings such as `clockAnalog`, Pomodoro intervals, and `sleepMin` survive simulator restarts without hardware connected.

---

## 7. Knowledge Base & Session Documentation (`docs/knowledge/`)

- Project knowledge documents reside in `docs/knowledge/`.
- **Skill**: Activate the `project-knowledge` skill (`.agents/skills/project-knowledge/SKILL.md`).
- **Reading Rule**: When consulting past decisions or features, read **ONLY** `docs/knowledge/index.md` first. Inspect the index table, select only the relevant topic doc(s), and do **not** dump all documents into context.
- **Recording Rule**: When asked to record session knowledge, create one or more dated docs (`YYYY-MM-DD-<slug>.md`) in `docs/knowledge/` separating topics cleanly, update `docs/knowledge/index.md`, and commit.

