# 2026-09-05: Digital Twin Workbench & Hardware Simulation Studio

## 1. Context & Motivation
Developing firmware for multi-sensor IoT devices with custom displays and haptics typically suffers from slow hardware upload/flash cycles and requires physical hardware on the developer's desk.
To accelerate firmware development, UI iteration, and end-to-end testing, Project Kubi incorporates a high-fidelity **Software-in-the-Loop (SITL) Digital Twin** that compiles production C++ firmware directly for the desktop alongside a companion React workbench.

---

## 2. Key Architecture & Design Decisions

### 1. Hardware Chassis Mockup & Auto-Orient Mode
- **Wooden Cube Chassis**: The workbench at [`http://localhost:5173/sim`](http://localhost:5173/sim) renders a faithful representation of the physical 10cm companion cube with warm wood grain styling and 4 corner brass machine screws.
- **Auto-Orient Mode**: When Face 2 (Pomodoro Timer) is selected, the chassis physically and smoothly rolls $-90^\circ$ onto its side into widescreen landscape ($320\times 240$) right-side up to the viewer sitting at their desk, mirroring physical cube rotation.
- **Glass Reflection & Screen Interaction**: The inner display bezel features a realistic glass glare gradient and an interactive canvas—clicking the screen directly triggers a simulated gentle tap (`POST /sim/inject {"gesture": "tap"}`).

### 2. Time of Day Scrubber & Simulated RTC
The firmware RTC drives two critical features:
1. **Face 1 (Clock Face)**: Large digital 7-segment clock digits, date layout, and calendar agenda.
2. **Face 3 (Mascot Routine)**: Jelly mascot routine and facial expressions:
   - `23:00 - 06:00` (**Sleep Routine**): Closed eyes with animated `"z Z"`.
   - `06:00 - 10:00` (**Morning Coffee Routine**): Wide happy eyes and smile.
   - `10:00 - 23:00` (**Focus Routine**): Focused working/reading eyes.

**Live vs. Manual Toggle**:
- A dedicated toggle button (`[ System Time: LIVE ]` $\leftrightarrow$ `[ Manual Override: ON ]`) enables switching between your host computer's wall clock and manual virtual time.
- Integrated with `getLocalTime()` in [`firmware/sim/include/Arduino.h`](file:///c:/Development/Kubi/firmware/sim/include/Arduino.h) via `sim_hour_override` and `sim_minute_override`.
- Quick-select preset buttons allow 1-click verification of routines: `02:00 Sleep`, `08:00 Coffee`, and `14:00 Focus`.

### 3. Balanced 2-Column Environment Layout
The Hardware Rig tab organizes sensor injection into a clean 2-column layout:
- **Left Column**: Stack of three control cards:
  1. **BMP280 Temperature**: Slider centered at $22.0^\circ\text{C}$ (Standard), spanning $10.0^\circ\text{C}$ to $34.0^\circ\text{C}$.
  2. **Battery Telemetry**: Level slider ($5\% - 100\%$) and percentage indicator.
  3. **Secret Override Alert**: Input field to test companion broadcast banners (`POST /api/override`).
- **Right Column**: Full-height **Time of Day Scrubber** card with live toggle, hour/minute sliders, and routine presets.
- **Full Width Below**: **ADXL345 3-Axis Accelerometer** vector injection controls.

### 4. Production Build Isolation & Dynamic Code-Splitting
- **ESP32 Silicon Build**: PlatformIO strictly compiles sources within `firmware/src/`. The simulator code in `firmware/sim/` and `firmware/include/httplib.h` is 100% excluded from `firmware.bin`.
- **Frontend Bundle Isolation**: [`dashboard/src/main.tsx`](file:///c:/Development/Kubi/dashboard/src/main.tsx) code-splits `Simulator.tsx` using `React.lazy()` and `Suspense`:
  ```tsx
  const Simulator = React.lazy(() => import("./pages/Simulator"));
  ```
  This isolates the Digital Twin workbench into an independent bundle chunk (`Simulator-*.js`, ~23 kB) that is never downloaded or parsed by users accessing the dashboard from the ESP32's local LittleFS web server.

### 5. C/C++ Preprocessor Comment Gotcha: Trailing Backslash Line Continuation
During early simulation runs, font glyphs $\ge \text{ASCII 97}$ ('a' through '|') shifted by $+1$ ASCII position (e.g. "Session" rendered as "Sfttjpo").
- **Root Cause**: In [`firmware/sim/src/TFT_eSPI_Mock.cpp`](file:///c:/Development/Kubi/firmware/sim/src/TFT_eSPI_Mock.cpp), line 179 contained:
  ```cpp
  0x02, 0x04, 0x08, 0x10, 0x20, // \
  0x00, 0x41, 0x41, 0x7F, 0x00, // ]
  ```
  In C/C++, a trailing backslash `\` at the end of a single-line comment acts as a preprocessor line continuation, deleting the subsequent line (`]`) from the compilation unit.
- **Fix**: Replaced `// \` with `// backslash`.
- **Rule**: Never end single-line comments with a trailing backslash.

---

## 3. Code Touchpoints & Files
- [`dashboard/src/pages/Simulator.tsx`](file:///c:/Development/Kubi/dashboard/src/pages/Simulator.tsx): Digital Twin UI, 3D/2D chassis styling, auto-orientation, time scrubber, and sensor feeds.
- [`dashboard/src/main.tsx`](file:///c:/Development/Kubi/dashboard/src/main.tsx): Dynamic lazy-loading route for `/sim`.
- [`dashboard/src/lib/audioChimes.ts`](file:///c:/Development/Kubi/dashboard/src/lib/audioChimes.ts): Web Audio 8-bit retro square-wave synthesizer.
- [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Desktop simulation runner and HTTP API server.
- [`firmware/sim/src/TFT_eSPI_Mock.cpp`](file:///c:/Development/Kubi/firmware/sim/src/TFT_eSPI_Mock.cpp): Framebuffer rasterizer.
- [`firmware/sim/include/Arduino.h`](file:///c:/Development/Kubi/firmware/sim/include/Arduino.h): Mock time, string, and hardware functions.
- [`firmware/sim/build_sim.ps1`](file:///c:/Development/Kubi/firmware/sim/build_sim.ps1): Clang++ compilation script for native runner.
- [`firmware/sim/run_sim.ps1`](file:///c:/Development/Kubi/firmware/sim/run_sim.ps1): One-command runner for C++ simulator and Vite dashboard.

---

## 4. Gotchas & Verification

> [!IMPORTANT]
> **Process Locking Protocol**:
> Windows locks running `.exe` binaries. When `kubi_sim.exe` is active in the user's terminal, running `build_sim.ps1` will fail with permission errors. When verifying Clang builds, output to a temporary binary (e.g. `kubi_sim_test.exe`). Never kill the user's running simulation process automatically.

### Verification Runbook
1. **Frontend Build Integrity**:
   ```bash
   cd dashboard
   npm run build
   ```
   Must compile cleanly with zero TypeScript errors and output chunk `Simulator-*.js`.
2. **Silicon Firmware Build**:
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
   ```
   Must verify that `firmware.bin` builds with 100% integrity.
3. **Clang++ Desktop Compilation Check**:
   ```powershell
   $clang = (Get-ChildItem "C:\Users\Mathys\AppData\Local\Microsoft\WinGet\Packages" -Recurse -Filter "clang++.exe" | Select-Object -First 1).FullName
   & $clang -std=c++17 -O2 -static -I firmware/sim/include -I firmware/include -I firmware/src -I firmware/.pio/libdeps/esp32dev/ArduinoJson/src firmware/src/PomodoroManager.cpp firmware/src/DisplayManager.cpp firmware/src/SensorManager.cpp firmware/src/AudioManager.cpp firmware/src/KubiSprites.cpp firmware/sim/src/TFT_eSPI_Mock.cpp firmware/sim/src/sim_main.cpp -lws2_32 -o firmware/sim/kubi_sim_test.exe
   Remove-Item firmware/sim/kubi_sim_test.exe
   ```
