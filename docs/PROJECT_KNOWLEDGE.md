# Project Kubi: Comprehensive Knowledge Base & Technical Reference

Welcome to the central engineering knowledge document for **Project Kubi** — an interactive, haptic ESP32 desk companion cube.

This document serves as the single source of truth for hardware specifications, firmware architecture, software-in-the-loop (SITL) simulation, user interaction models, audio synthesis, and companion web dashboard integration.

---

## 1. System Overview & Core Philosophy

- **Target Persona**: Wilhelm ("W") — 21st birthday gift.
- **Physical Chassis**: 10cm × 10cm × 10cm (10cm³) handcrafted wooden shell with internal 3D-printed mounting chassis and brass machine screws.
- **"No-Button" Rule**: The companion cube has no physical buttons. All user navigation, mode selection, and interactions are driven exclusively by physical manipulation (cube orientation, gentle taps, shakes, and desk slams).
- **Zero Code Duplication**: Core firmware logic is shared 1:1 between real ESP32 silicon and the native desktop hardware simulator (`firmware/sim/`).
- **Offline & Low Maintenance**: Configuration and state persist to non-volatile storage (ESP32 NVS Preferences / `kubi_sim_prefs.txt`), and the companion dashboard is served directly from onboard LittleFS flash memory.

---

## 2. Hardware Architecture & Pin Map

| Subsystem | Component | Bus / Protocol | ESP32 GPIO | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Brain** | ESP32-WROOM-32 (30-pin) | Dual-Core 240MHz | N/A | Core 0: Network & Web; Core 1: Hardware & UI |
| **Display** | 2.0" IPS LCD (ST7789, 240×320) | SPI | SCL: `18`, SDA: `23`, CS: `5`, DC: `2`, RST: `4` | Wide viewing angles, 16-bit RGB565 color |
| **Backlight** | ST7789 LED Backlight | PWM | `15` | Duty cycle 0–255, night mode dimming |
| **Motion & Temp** | DF Robot 10 DOF (ADXL345 + BMP280)| I2C (Wire) | SDA: `21`, SCL: `22` | Orientation, tap, shake, slam, room temp |
| **Audio** | MAX98357A I2S 3W Class D Amp | I2S | BCLK: `26`, LRC: `25`, DIN: `27` | 8-bit square-wave synth over I2S |
| **Speaker** | 3W 8Ω Enclosed Speaker | Analog Output | N/A | Built-in acoustic chamber for punchy retro sound |
| **Power** | 3× 18650 Li-ion Cells (~10,000mAh) | IP5328P / IP5306 | VIN / 5V | Pass-through USB-C charging, multi-day battery life |

---

## 3. Dual-Core Threading Architecture (FreeRTOS)

```
                       ┌──────────────────────────────────────────────┐
                       │                ESP32 CPU                     │
                       ├──────────────────────┬───────────────────────┤
                       │       Core 0         │        Core 1         │
                       │ (Network & Services) │   (Hardware & UI)     │
                       ├──────────────────────┼───────────────────────┤
                       │ • WiFi / AP Portal   │ • ADXL345 Polling     │
                       │ • ESPAsyncWebServer  │ • BMP280 Temp Loop    │
                       │ • LittleFS Static UI │ • Orientation FSM     │
                       │ • iCal Client (Sync) │ • TFT_eSPI Graphics   │
                       │ • REST API Endpoints │ • AudioManager (I2S)  │
                       │ • Background Tasks   │ • Pomodoro Ticks      │
                       └──────────────────────┴───────────────────────┘
```

1. **Core 0 (`setup()` & Async Tasks)**:
   - Wi-Fi management (`WiFiManager`).
   - Async Web Server (`ESPAsyncWebServer`): serves gzipped React dashboard from LittleFS and handles `/api/state`, `/api/settings`, `/api/pomodoro/action`, and `/api/override`.
   - Periodic calendar sync: pulls Google Calendar `.ics`, parses next 3 days into memory.
2. **Core 1 (`core1HardwareTask` / ~50–60Hz Loop)**:
   - High-frequency sensor polling (`sensors.loop()`).
   - Audio buffer synthesis (`audio.loop()`).
   - Pomodoro 1-second countdown tick (`pomodoro.tick()`).
   - Orientation evaluation & debounce (`sensors.getActiveFace()`).
   - Gesture event dispatcher (Tap, Shake, Slam).
   - Display rendering (`DisplayManager` at ~20–25Hz).

---

## 4. Software-in-the-Loop (SITL) Digital Twin

The codebase includes a desktop simulation environment (`firmware/sim/`) that runs real C++ firmware on native x86_64 Windows without requiring physical ESP32 hardware.

### 4.1 Architecture & HAL Isolation
- Core files shared 1:1:
  - `firmware/src/PomodoroManager.cpp`
  - `firmware/src/DisplayManager.cpp`
  - `firmware/src/SensorManager.cpp`
  - `firmware/src/AudioManager.cpp`
  - `firmware/src/KubiSprites.cpp`
- **Desktop Mocks (`firmware/sim/include/`)**:
  - `Arduino.h`: Emulates `millis()`, `delay()`, `Serial`, and mathematical primitives.
  - `TFT_eSPI_Mock.cpp`: Rasterizes graphics, 5×7 ASCII fonts, and 7-segment timer digits into a memory buffer (`sim_framebuffer`, 240×320×4 RGBA).
  - `Preferences.h`: Emulates ESP32 NVS Preferences API and persists key-value pairs to `kubi_sim_prefs.txt`.
  - `AudioOutputI2S.h`: Stubbed I2S output.
  - `sim_main.cpp`: Multi-threaded runner launching the Core 1 hardware simulation loop in a background thread and hosting an HTTP server (`cpp-httplib`) on port 8080.

### 4.2 Simulator HTTP API (Port 8080)
- `GET /sim/frame`: Raw 240×320 32-bit RGBA pixel buffer stream consumed by the simulator canvas.
- `GET /sim/state`: Telemetry heartbeat (orientation, rotation, backlight PWM, last chime event, active mode).
- `POST /sim/inject`: Injects virtual accelerometer coordinates, face selection, gestures, temperature, and time machine overrides.
- `GET /api/state`: Returns system state (identical schema to real ESP32 firmware).
- `POST /api/settings`: Updates Pomodoro durations, colors, calendar URL, and clock mode.
- `POST /api/pomodoro/action`: Triggers `play`, `pause`, `toggle`, `skip`, or `reset`.
- `POST /api/override`: Sends immediate companion alert banner across the screen.

### 4.3 Simulator Execution Protocols (CRITICAL)
- **Do NOT launch or kill `kubi_sim.exe` automatically**: The user runs `kubi_sim.exe` in an external terminal.
- **File Locking**: Running Windows processes lock the executable binary on disk. Running `build_sim.ps1` while `kubi_sim.exe` is running fails with file lock errors. When testing clang++ builds, always compile to a temporary binary (e.g. `kubi_sim_test.exe`).

---

## 5. Face Modes & Functional Specifications

The front display adapts dynamically based on which of the four side faces rests facing "UP":

```
         ┌───────────────────┐
         │ Face 1: CLOCK     │
  ┌──────┴───────────────────┴──────┐
  │ Face 2: POMODORO                │ (Rotates -90° to landscape)
  ├─────────────────────────────────┤
  │ Face 3: MASCOT & TEMP           │ (Portrait upright)
  ├─────────────────────────────────┤
  │ Face 4: SCHEDULE AGENDA         │ (Rotates +90° to landscape)
  └─────────────────────────────────┘
```

### Face 1: Minimal Focus Clock (Face 1 UP)
- **Digital Mode (Default)**: Clean minimalist time display with large 7-segment numbers centered on screen. Date displayed formatted as `[Day, D Mon]` (e.g. `Thu, 3 Sep`) 40px below the time in Font 2.
- **Analog Mode**: Clean radial analog clock with 12 numbers, 3px orange hour hand, 2px white minute hand, center pivot, and date below without any circular border.
- **Gesture**: Shake (`GESTURE_SHAKE`) toggles between digital 7-segment and analog modes.
- **Persistence**: Choice is saved in NVS Preferences (`"kubi_settings"` -> `"clockAnalog"`).

### Face 2: Pomodoro Productivity Timer (Face 2 UP)
- **Landscape Layout**: Chassis rolls $-90^\circ$ onto its side into widescreen $320\times 240$.
- **Not Started / Idle State**:
  - `_isStarted == false`.
  - Timer sits at initial duration (e.g., 25:00) and **does not count down**.
  - Displays a slow retro arcade-style flashing `"START"` text in white (~1.2s period: 600ms on, 600ms off).
  - Tapping the cube, clicking the screen, or clicking "Start" on the dashboard kicks off the countdown (`_isStarted = true`).
- **Running Countdown State**:
  - Phase banner (`FOCUS`, `SHORT BREAK`, `LONG BREAK`) in configured color.
  - Session indicator (`Session 1 of 4`).
  - Giant 7-segment digits.
  - Clean progress bar at bottom (`h - 18`).
  - No cluttering instruction text.
- **Paused State**:
  - Centered solid white `"PAUSED"` text above the progress bar.
- **Pause State Persistence**:
  - When paused, moving the cube to another face and returning preserves the paused state without auto-resuming. Face transitions must never unconditionally call `pomodoro.play()`.
- **Reset Action**:
  - Resets duration to `_totalSeconds`, sets `_isStarted = false`, `_isPaused = false`, and restores the flashing `"START"` prompt.

### Face 3: Mascot Routine & Room Temp (Face 3 UP)
- Animated bouncing Kubi jelly character.
- Live room temperature readings from BMP280 in degrees Celsius.
- Time-of-day routines:
  - Sleep (`Zzz`): 23:00–06:00
  - Morning Coffee: 06:00–10:00
  - Focus Reading: 10:00–23:00

### Face 4: Schedule Agenda (Face 4 UP)
- Displays Google Calendar agenda items parsed from iCal URL.
- Gentle tap cycles pages or tags.

---

## 6. Physical Gestures & False Tap Suppression

### Gesture Classification
- **Gentle Tap (`GESTURE_TAP`)**: Sharp acceleration transient ($7.0\text{ m/s}^2 < \Delta\text{Mag} < 30.0\text{ m/s}^2$) with debounce ($>350\text{ms}$). Toggles clock details, starts/pauses/resumes Pomodoro, silences chimes, or cycles agenda pages.
- **Shake (`GESTURE_SHAKE`)**: Rapid sign reversals in X/Y axes ($\ge 3$ reversals within 500ms window). Toggles clock analog/digital view, skips Pomodoro phase.
- **Desk Slam (`GESTURE_SLAM`)**: High-G impulse ($\text{Total Mag} > 35.0\text{ m/s}^2$). Triggers "Ouch!" Easter egg chime.

### False Tap Suppression Protocol
- Rolling or flipping the cube produces high transient accelerations ($\Delta\text{Mag} > 7.0\text{ m/s}^2$).
- In `SensorManager::updateFace()`:
  - When candidate face changes: update `_lastTapTime = millis()` and reset `_recentGesture = GESTURE_NONE`.
  - When orientation settles (after 400ms debounce): update `_lastTapTime = millis()` and reset `_recentGesture = GESTURE_NONE`.
- In `main.cpp` and `sim_main.cpp`:
  - When `activeFace != previousFace`: drain/discard any lingering gestures via `sensors.getRecentGesture()`.

---

## 7. Chime Audio Palette & Sound Synthesis

Audio is synthesized over I2S to the MAX98357A amplifier in hardware, and through an authentic 8-bit square-wave synthesizer in the desktop simulator.

| Chime Enum | Melodic Sequence | Durations | Usage Context |
| :--- | :--- | :--- | :--- |
| `CHIME_TAP_FEEDBACK` | A5 (880 Hz) | 35ms | Gentle tap confirmation, button feedback |
| `CHIME_WAKE_PING` | D5 (587 Hz) $\to$ A5 (880 Hz) | 60ms, 100ms | Bootup, waking from night sleep, phase skip |
| `CHIME_POMODORO_DONE` | C5 (523 Hz) $\to$ E5 (659 Hz) $\to$ G5 (784 Hz) $\to$ C6 (1046 Hz) | 100ms, 100ms, 100ms, 250ms (~550ms) | Focus session finish into Short Break |
| `CHIME_POMODORO_LONG_BREAK` | C5 (523 Hz) $\to$ E5 (659 Hz) $\to$ G5 (784 Hz) $\to$ C6 (1046 Hz) $\to$ D6 (1175 Hz) $\to$ E6 (1318 Hz) $\to$ G6 (1568 Hz) | 90ms, 90ms, 90ms, 130ms, 100ms, 120ms, 350ms (~970ms) | Triumphant 7-note fanfare when all cycles finish into Long Break |
| `CHIME_SLAM_OUCH` | F4 (349 Hz) $\to$ C4 (262 Hz) $\to$ F3 (175 Hz) | 80ms, 80ms, 160ms | Desk slam Easter egg reaction |

---

## 8. Build, Test & Verification Runbook

Before submitting or committing any firmware or companion changes:

1. **Verify Real Hardware Build (PlatformIO)**:
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
   ```
   Must exit with code `0` and output `firmware.bin`.

2. **Verify Companion Web Dashboard (Vite / TypeScript)**:
   ```powershell
   cd dashboard
   npm run build
   ```
   Must pass type checks and bundle assets into `firmware/data/`.

3. **Verify Desktop Simulator Compilation (clang++)**:
   ```powershell
   $firmwareDir = "c:\Development\Kubi\firmware"
   $scriptDir = "c:\Development\Kubi\firmware\sim"
   $clang = (Get-ChildItem "C:\Users\Mathys\AppData\Local\Microsoft\WinGet\Packages" -Recurse -Filter "clang++.exe" -ErrorAction SilentlyContinue | Select-Object -First 1).FullName
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
   $outTest = "$scriptDir\kubi_sim_test.exe"
   & $clang -std=c++17 -O2 -static $includes $srcFiles -lws2_32 -o $outTest
   if ($LASTEXITCODE -eq 0) { Remove-Item -Force $outTest; Write-Host "CLANG_SIM_OK" }
   ```
