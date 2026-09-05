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
2. **Keep Core Firmware Pure**:
   - Do not add desktop-specific `#ifdef` hacks directly into `firmware/src/*.cpp` unless necessary. Keep simulation mocks isolated inside `firmware/sim/`.
3. **C/C++ Preprocessor Comment Gotcha**:
   - Never end a single-line comment with a trailing backslash (`// \`). In standard C/C++, this joins the next line to the comment as a line continuation, deleting whatever was on that line!
4. **When Modifying Frontend**:
   - Always run `npm run build` in `dashboard/` to verify TypeScript types and Vite build integrity.
