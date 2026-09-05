# 2026-09-06: High-Level Project Architecture, Heritage, and System Structure

## 1. Context & Motivation

Project Kubi is an interactive, haptic desk companion cube built as a 21st birthday gift for Wilhelm (mid-November 2026 deadline). The physical cube is a 10cm³ wooden chassis with an internal 3D-printed skeleton housing an ESP32 microcontroller, a 2.0" 240×320 ST7789 IPS LCD, an ADXL345 3-axis accelerometer, a BMP280 environmental sensor, and a MAX98357A I2S Class D amplifier with miniature speaker.

### Heritage from Project Sunrise
The codebase originated as a direct structural duplication of **Project Sunrise** (a custom smart ambient wake-up lamp built on ESP32 with FastLED WWA lighting, a TSL2591 light sensor, and a Dusk2Dawn solar astronomy engine).

While the physical actuators and peripherals have completely shifted (LED channel PWM $\rightarrow$ IPS display & I2S audio; light sensor $\rightarrow$ motion & barometric sensors), the **high-level structure, footprint, and architectural conventions of Project Sunrise were intentionally preserved**:
- **Dual-Core FreeRTOS Model:** Core 0 runs networking and web servers; Core 1 handles real-time sensor loops and display rendering.
- **LittleFS Web App Bundle:** The frontend is a single-page React app compiled into `firmware/data` and served directly by `ESPAsyncWebServer` from the ESP32 flash partition.
- **RESTful API Footprint:** Predictable JSON telemetry endpoints (`/api/state`), configuration persistence (`/api/settings`), and companion interactions (`/api/override`).
- **Mobile-First Companion UI:** The signature `SkyArc` header, dark zinc aesthetics, dynamic contextual card hierarchy, and collapsible settings accordions.
- **Persistent Storage:** NVS key-value storage (`Preferences.h`) for user configurations (timer lengths, color themes, display preferences).

---

## 2. Key Architecture & Design Decisions

### 2.1 The "No-Buttons" Interaction Paradigm
Kubi has zero external pushbuttons, switches, or capacitive touch areas on its exterior. All interactions are driven by physics:
1. **Primary Navigation via Orientation:** 4 resting faces determine the operating mode:
   - **Face 1 (Clock Face):** Minimalist 7-segment digital time, date (`Thu, 3 Sep`), and shake-activated analog clock toggle.
   - **Face 2 (Pomodoro Timer):** Work, Short Break, Long Break countdowns, cycle progress, and arcade-style flashing `START` idle state.
   - **Face 3 (Mascot & Room Temp):** Animated bouncing jelly companion and live room temperature readings.
   - **Face 4 (Schedule Agenda):** 3-day Google Calendar agenda fetched via background iCal parser.
2. **Contextual In-Mode Gestures (ADXL345 Accelerometer):**
   - **Gentle Tap:** Primary action (dismiss chime, toggle Pomodoro play/pause, cycle agenda events).
   - **Shake:** Secondary action (skip to next Pomodoro phase, toggle analog/digital clock view).
   - **Desk Slam (>3.5G):** Easter egg shock reaction (mascot wince, error buzz).

### 2.2 Dual-Target Software-in-the-Loop (SITL) Architecture
To allow rapid UI and feature development without physical hardware, Kubi features zero-duplication simulation:
- **Shared Firmware Logic (1:1):**
  - [`DisplayManager.cpp`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp)
  - [`SensorManager.cpp`](file:///c:/Development/Kubi/firmware/src/SensorManager.cpp)
  - [`PomodoroManager.cpp`](file:///c:/Development/Kubi/firmware/src/PomodoroManager.cpp)
  - [`AudioManager.cpp`](file:///c:/Development/Kubi/firmware/src/AudioManager.cpp)
  - [`KubiSprites.cpp`](file:///c:/Development/Kubi/firmware/src/KubiSprites.cpp)
- **Desktop Hardware Abstraction Layer (`firmware/sim/`):**
  - `TFT_eSPI_Mock.cpp` rasterizes display draw calls into a raw 240×320 32-bit RGBA pixel buffer.
  - `Preferences.h` emulates ESP32 NVS flash using a text mirror (`kubi_sim_prefs.txt`).
  - Desktop runner (`sim_main.cpp`) exposes an HTTP API (`:8080`) streaming frames and accepting virtual hardware injections.
- **Web Digital Twin Studio (`dashboard/src/pages/Simulator.tsx`):**
  - 3D CSS rendering of the wooden cube with brass screws and realistic glass glare reflection.
  - Automatic chassis roll into widescreen landscape mode on Face 2/Face 4.
  - Real-time 8-bit Web Audio synthesizer recreating square-wave chimes.

### 2.3 Automated Asset Pipeline (`build_sprites.py`)
- Drop animated `.gif` or `.png` files into `firmware/assets/sprites/`.
- PlatformIO SCons pre-build hook (`extra_scripts = pre:tools/build_sprites.py`) slices images into RGB565 byte arrays stored in `PROGMEM`, requiring 0 bytes of dynamic RAM.

### 2.4 Partitioning Table (`firmware/partitions.csv`)
Standard ESP32 1.25MB app partitions overflow when bundling `TFT_eSPI`, `ESP8266Audio`, and `ESPAsyncWebServer`. The partition table was customized:
- `app0`: 2.0MB (firmware binary)
- `spiffs` (LittleFS): 1.8MB (compressed dashboard bundle)
- `nvs`: 32KB (persistent user settings)

---

## 3. Code Touchpoints & Files

### Firmware Layer (`firmware/`)
- [`firmware/src/main.cpp`](file:///c:/Development/Kubi/firmware/src/main.cpp): FreeRTOS dual-core initialization, Wi-Fi handling, main loops.
- [`firmware/src/DisplayManager.h`](file:///c:/Development/Kubi/firmware/src/DisplayManager.h) & [`DisplayManager.cpp`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp): Display rendering, coordinate transformations, face layouts.
- [`firmware/src/SensorManager.h`](file:///c:/Development/Kubi/firmware/src/SensorManager.h) & [`SensorManager.cpp`](file:///c:/Development/Kubi/firmware/src/SensorManager.cpp): ADXL345 orientation classifier, tap/shake/slam gesture detection, BMP280 temperature reader.
- [`firmware/src/PomodoroManager.h`](file:///c:/Development/Kubi/firmware/src/PomodoroManager.h) & [`PomodoroManager.cpp`](file:///c:/Development/Kubi/firmware/src/PomodoroManager.cpp): Pomodoro state machine, idle state, phase countdowns, NVS synchronization.
- [`firmware/src/AudioManager.h`](file:///c:/Development/Kubi/firmware/src/AudioManager.h) & [`AudioManager.cpp`](file:///c:/Development/Kubi/firmware/src/AudioManager.cpp): I2S tone generator, chime melodies (victory arpeggios, long break fanfare, alerts).
- [`firmware/src/API.h`](file:///c:/Development/Kubi/firmware/src/API.h) & [`API.cpp`](file:///c:/Development/Kubi/firmware/src/API.cpp): REST endpoints for state, settings, Pomodoro actions, and secret overrides.
- [`firmware/partitions.csv`](file:///c:/Development/Kubi/firmware/partitions.csv): 2MB App / 1.8MB LittleFS partition table.
- [`firmware/tools/build_sprites.py`](file:///c:/Development/Kubi/firmware/tools/build_sprites.py): Pre-build Pillow sprite slicing script.

### Digital Twin & Simulation Layer (`firmware/sim/`)
- [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Desktop simulation runner and HTTP API.
- [`firmware/sim/src/TFT_eSPI_Mock.cpp`](file:///c:/Development/Kubi/firmware/sim/src/TFT_eSPI_Mock.cpp): Framebuffer rasterizer.
- [`firmware/sim/include/Preferences.h`](file:///c:/Development/Kubi/firmware/sim/include/Preferences.h): File-backed NVS mock (`kubi_sim_prefs.txt`).
- [`firmware/sim/build_sim.ps1`](file:///c:/Development/Kubi/firmware/sim/build_sim.ps1): Clang++ compilation script for Windows.

### Dashboard Layer (`dashboard/`)
- [`dashboard/src/App.tsx`](file:///c:/Development/Kubi/dashboard/src/App.tsx): Companion dashboard, state polling, contextual hero card, settings accordion.
- [`dashboard/src/components/SkyArc.tsx`](file:///c:/Development/Kubi/dashboard/src/components/SkyArc.tsx): Header arc visualization.
- [`dashboard/src/components/Accordion.tsx`](file:///c:/Development/Kubi/dashboard/src/components/Accordion.tsx): Settings panel accordion container.
- [`dashboard/src/pages/Simulator.tsx`](file:///c:/Development/Kubi/dashboard/src/pages/Simulator.tsx): 3D Wooden cube Digital Twin workbench.
- [`dashboard/src/pages/Manual.tsx`](file:///c:/Development/Kubi/dashboard/src/pages/Manual.tsx): User manual documentation.

---

## 4. Gotchas & Verification

1. **Simulator Process Locking:**
   - The user runs `kubi_sim.exe` externally in a dedicated terminal.
   - Do **NOT** launch or terminate `kubi_sim.exe` automatically.
   - Windows locks active binaries. When testing simulator builds, output to a temporary binary (`kubi_sim_test.exe`).
2. **False Tap Suppression:**
   - Flipping or rolling the cube creates transient acceleration deltas ($>7.0g$).
   - `SensorManager::updateFace()` resets `_lastTapTime = millis()`, drains pending gestures, and requires orientation stability before enabling tap recognition.
3. **PowerShell UTF-8 BOM Issue:**
   - PowerShell `Set-Content -Encoding utf8` injects a byte-order mark (`\ufeff`), which corrupts PlatformIO config files (`platformio.ini`). Use raw .NET UTF8 encoding without BOM when writing configuration files.
4. **Verification Commands:**
   - **Firmware Compilation:**
     ```powershell
     & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
     ```
   - **Dashboard Compilation:**
     ```powershell
     cd dashboard
     npm run build
     ```
