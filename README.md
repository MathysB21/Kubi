# Kubi (Cube) — Smart Haptic ESP32 Desk Companion

<div align="center">

```
             +-----------------------+
             |     FACE 1: CLOCK     |
      +------*-----------------------*------+
     /                                       \
    +   FACE 2: POMODORO    FACE 4: SCHEDULE  +
     \                                       /
      +------*-----------------------*------+
             |    FACE 3: MASCOT     |
             +-----------------------+
```

**A handcrafted, zero-button, physics-driven smart desk companion cube.**  
Featuring a 2.0" IPS display, retro 8-bit chimes, dual-core FreeRTOS firmware, an authentic Software-in-the-Loop (SITL) Digital Twin, and an embedded React companion dashboard.

[![ESP32](https://img.shields.io/badge/Platform-ESP32--WROOM--32-E7352C?logo=espressif&logoColor=white)](#4-hardware-architecture--pinout)
[![FreeRTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20Dual--Core-0085CA)](#5-dual-core-threading-architecture)
[![TFT_eSPI](https://img.shields.io/badge/Display-ST7789%20240x320%20IPS-4B8BBE)](#4-hardware-architecture--pinout)
[![React](https://img.shields.io/badge/Companion-React%2019%20%7C%20Vite%20%7C%20Tailwind%20v4-61DAFB?logo=react&logoColor=black)](#7-companion-web-dashboard)
[![SITL Digital Twin](https://img.shields.io/badge/Simulation-SITL%20Digital%20Twin-10B981)](#6-software-in-the-loop-sitl-digital-twin)
[![Zero Buttons](https://img.shields.io/badge/Interaction-100%25%20Physics%20%26%20Gestures-F59E0B)](#the-no-button-interaction-paradigm)

</div>

---

## 1. Overview & Core Philosophy

**Project Kubi** is an interactive, haptic desk companion cube built as a 21st birthday gift for Wilhelm ("W"). It sits on a desk inside a 10cm³ handcrafted hardwood enclosure with an internal 3D-printed skeleton and brass machine screws.

### The "No-Button" Interaction Paradigm
Kubi has **zero physical buttons, switches, or capacitive pads**. All interactions and navigations are driven purely by physical manipulation detected by an onboard 3-axis accelerometer (ADXL345):
- **Orientation-Based Modes**: Flipping the cube so that one of its four side faces rests facing **UP** selects the active operating mode.
- **Dynamic Orientation Roll**: The front 2.0" IPS screen remains visible facing the user, automatically pivoting between portrait ($240\times 320$) and widescreen landscape ($320\times 240$) right-side up.
- **Physical Gestures**:
  - **Gentle Tap**: Contextual primary action (toggle Pomodoro start/pause, advance schedule pages, dismiss chimes).
  - **Shake**: Secondary contextual action (skip Pomodoro phase, toggle clock analog/digital view, jump back to today's schedule).
  - **Desk Slam (>3.5G)**: Easter egg shock detection (triggers an "ouch!" chime and wincing mascot reaction).

### Zero Code Duplication
Kubi uses a **Software-in-the-Loop (SITL)** architecture: core C++ firmware logic (`DisplayManager`, `PomodoroManager`, `SensorManager`, `AudioManager`, `ScheduleManager`, `KubiSprites`) is shared **1:1** between physical ESP32 silicon and a native desktop simulation runner running on x86_64.

---

## 2. Operating Modes (Face Modes)

```
       ┌───────────────────────────────┐
       │ Face 1: MINIMAL FOCUS CLOCK   │ (Portrait Upright)
┌──────┴───────────────────────────────┴──────┐
│ Face 2: POMODORO PRODUCTIVITY TIMER         │ (Landscape -90°)
├─────────────────────────────────────────────┤
│ Face 3: MASCOT ROUTINE & ROOM ENVIRONMENT   │ (Portrait Upright)
├─────────────────────────────────────────────┤
│ Face 4: SCHEDULE & AGENDA CALENDAR          │ (Landscape +90°)
└─────────────────────────────────────────────┘
```

### Face 1: Minimal Focus Clock (Face 1 UP)
* **Digital Mode (Default)**: Large, high-contrast 7-segment clock centered on screen with the date formatted as `[Day, D Mon]` (e.g. `Thu, 3 Sep`) 40px below in clean typography.
* **Analog Mode**: Clean radial analog face with 12 hour numbers, 3px orange hour hand, 2px white minute hand, center pivot, and date below, without cluttering outer circular borders.
* **Shake Gesture (`GESTURE_SHAKE`)**: Instantly toggles between digital 7-segment and analog views.
* **Persistent Preferences**: Mode selection is permanently remembered across reboots and simulator restarts via ESP32 NVS (`Preferences.h` / `"kubi_settings"` -> `"clockAnalog"`).

### Face 2: Pomodoro Productivity Timer (Face 2 UP)
* **Auto-Orient Landscape**: Cube rolls onto its side into widescreen $320\times 240$ right-side up.
* **Arcade "START" Idle State**: When reset or not yet started, the timer sits paused at full duration (e.g. 25:00) with a retro arcade-style slow-flashing `"START"` prompt in white (~1.2s period). It will not count down until intentionally triggered.
* **Play / Pause / Resume**:
  - Tapping the cube or clicking "Start" on the dashboard kicks off the countdown.
  - Tapping while running immediately pauses the timer and renders clean, centered solid white `"PAUSED"` text above the progress bar (no cluttering instruction text).
  - Tapping while paused resumes the session.
* **Phase Navigation**: Shaking the cube skips immediately to the next Pomodoro phase.
* **Pause State Persistence**: Navigating away from Face 2 while paused preserves the paused state upon return without auto-resuming.
* **Audio Rewards**: Ascending 4-note C-major arpeggio on regular session completion; celebratory 7-note triumphant fanfare (`CHIME_POMODORO_LONG_BREAK`) upon completing all cycles into the Long Break.

### Face 3: Mascot Routine & Room Environment (Face 3 UP)
* **Kubi Mascot**: Animated bouncing jelly companion rendered from byte arrays in `PROGMEM` with zero dynamic RAM allocation.
* **Time-of-Day Contextual Routines**:
  - `23:00 – 06:00`: Sleep routine (`Zzz` animation)
  - `06:00 – 10:00`: Morning Coffee routine
  - `10:00 – 23:00`: Focus Reading routine
* **Live Environmental Telemetry**: Real-time room temperature readings sampled from the onboard BMP280 barometric sensor.

### Face 4: Schedule Agenda (Face 4 UP)
* **Google Calendar Integration**: Background RFC-5545 `.ics` client fetches and parses calendar events for Today + 4 upcoming days (5-day ceiling).
* **Dynamic Relative Day Headers**: Clear pink typography displaying relative day labels (`Schedule (today)`, `Tomorrow`, `Thu, 10 Sep`).
* **Strict 4-Item Screen Pagination**:
  - Displays exactly 4 event cards per screen.
  - **Gentle Tap**: Advances through sub-pages of the current day. Once the day's events are exhausted, advances to the next day.
  - **Shake Gesture**: Immediately returns to `Schedule (today)` with auditory chime confirmation.
* **Embedded Finance Ticker Marquee**: Long event titles scroll smoothly across the card using modular arithmetic while the start time and dash (e.g. `14:00 - `) remain statically pinned on the left.
* **Dual-Side Masking**: Zero-overhead hardware-agnostic clipping technique that cleanly masks marquee text without requiring expensive framebuffers or memory-hungry sprites.
* **Empty State Handling**: Displays contextual empty state cards when no events are scheduled.

---

## 3. Physical Gestures & Audio Palette

### Accelerometer Gestures (ADXL345)
| Gesture | Signal Profile | Debounce / Threshold | Contextual Action |
| :--- | :--- | :--- | :--- |
| **Gentle Tap** | $7.0\text{ m/s}^2 < \Delta\text{Mag} < 30.0\text{ m/s}^2$ | $>350\text{ms}$ cooldown | Toggle Pomodoro start/pause, advance schedule pages, dismiss chimes |
| **Shake** | Rapid X/Y sign reversals | $\ge 3$ reversals in 500ms window | Toggle clock analog/digital, skip Pomodoro phase, jump to today's schedule |
| **Desk Slam** | High-G shock impulse | $\text{Total Mag} > 35.0\text{ m/s}^2$ (>3.5G) | Easter egg shock reaction (mascot wince, ouch chime) |

> [!NOTE]
> **False Tap Suppression Protocol**: Rolling or flipping the cube produces high transient accelerations ($\Delta\text{Mag} > 7.0\text{ m/s}^2$). `SensorManager` updates `_lastTapTime = millis()` on candidate transitions and orientation settling, and drains lingering gestures on face changes to prevent accidental taps during orientation switches.

### Retro 8-Bit Audio Chimes
Audio is synthesized over I2S to a MAX98357A amplifier in hardware, and through a real-time Web Audio square-wave synthesizer in the desktop simulator.

| Chime Identifier | Melodic Notes | Durations | Trigger Event |
| :--- | :--- | :--- | :--- |
| `CHIME_TAP_FEEDBACK` | `A5` (880 Hz) | 35ms | Gentle tap confirmation, interaction feedback |
| `CHIME_WAKE_PING` | `D5` (587 Hz) $\to$ `A5` (880 Hz) | 60ms, 100ms | Bootup, wake from sleep, phase skip |
| `CHIME_POMODORO_DONE` | `C5` $\to$ `E5` $\to$ `G5` $\to$ `C6` | 100ms each, 250ms end (~550ms) | Work session completed / Short Break |
| `CHIME_POMODORO_LONG_BREAK` | `C5` $\to$ `E5` $\to$ `G5` $\to$ `C6` $\to$ `D6` $\to$ `E6` $\to$ `G6` | 90–350ms (~970ms) | Triumphant 7-note fanfare on Long Break transition |
| `CHIME_SLAM_OUCH` | `F4` (349 Hz) $\to$ `C4` (262 Hz) $\to$ `F3` (175 Hz) | 80ms, 80ms, 160ms | Desk slam Easter egg reaction |

---

## 4. Hardware Architecture & Pinout

```
                           +------------------------+
                           |  ESP32-WROOM-32 (30P)  |
                           +------------------------+
                               |     |     |     |
              +----------------+     |     |     +----------------+
              | (SPI)                |     |                (I2S) |
              v                      |     |                      v
      +---------------+              |     |              +---------------+
      |  2.0" ST7789  |              |     |              |   MAX98357A   |
      | 240x320 IPS   |              |     |              | Class D Amp   |
      +---------------+              |     |              +---------------+
                                     |     |                      |
                    (PWM Backlight)  |     | (I2C Bus)            v
                    GPIO 15 ---------+     +---- GPIO 21 (SDA)   +--------+
                                           +---- GPIO 22 (SCL)   | 3W 8Ω  |
                                                   |             | Speaker|
                                            +------+------+      +--------+
                                            |             |
                                            v             v
                                      +----------+  +----------+
                                      | ADXL345  |  |  BMP280  |
                                      |  Motion  |  |   Temp   |
                                      +----------+  +----------+
```

### GPIO Pin Mapping Table
| ESP32 Pin | Subsystem | Module Pin | Protocol / Function | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`VIN` / `5V`** | Power | `VIN` | 5V DC | 5V rail from battery power bank module |
| **`3V3`** | Power | `VCC` | 3.3V DC | Regulated logic power for sensors & display |
| **`GND`** | Power | `GND` | Common Ground | Common ground across all modules |
| **`GPIO 18`** | Display | `SCL` / `CLK` | SPI Clock | Hardware SPI Clock (`SCK`) |
| **`GPIO 23`** | Display | `SDA` / `MOSI`| SPI MOSI | Hardware SPI Master Out Slave In |
| **`GPIO 5`** | Display | `CS` | SPI Chip Select | Display controller select |
| **`GPIO 2`** | Display | `DC` | Control | Data / Command select |
| **`GPIO 4`** | Display | `RST` | Reset | Display hardware reset |
| **`GPIO 15`** | Display | `BLK` / `LED` | PWM Backlight | Backlight brightness / sleep mode dimming |
| **`GPIO 21`** | Sensors | `SDA` | I2C Data | Hardware I2C data bus (ADXL345 + BMP280) |
| **`GPIO 22`** | Sensors | `SCL` | I2C Clock | Hardware I2C clock bus (ADXL345 + BMP280) |
| **`GPIO 26`** | Audio | `BCLK` | I2S Bit Clock | Serial data clock for MAX98357A |
| **`GPIO 25`** | Audio | `LRC` | I2S Word Select | Left/Right channel frame clock |
| **`GPIO 27`** | Audio | `DIN` | I2S Data In | Serial audio data line |

### Power Supply & Flash Partitions
* **Battery Subsystem**: 3× 18650 Li-ion cells wired in parallel (~10,000mAh total) managed by an IP5328P / IP5306 module providing pass-through USB-C charging and multi-day battery life.
* **Custom Partition Table (`firmware/partitions.csv`)**:
  - `app0`: **2.0 MB** (compiled firmware binary with TFT_eSPI and ESP8266Audio)
  - `spiffs` (LittleFS): **1.8 MB** (gzipped companion web dashboard and static assets)
  - `nvs`: **32 KB** (persistent configuration and calibration)

---

## 5. Dual-Core Threading Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           ESP32 Dual-Core CPU                           │
├────────────────────────────────────┬────────────────────────────────────┤
│              Core 0                │               Core 1               │
│      (Network, Web & Services)     │       (Hardware, UI & Audio)       │
├────────────────────────────────────┼────────────────────────────────────┤
│ • Wi-Fi Manager & Auto-Reconnect   │ • ADXL345 High-Freq Polling (50Hz) │
│ • ESPAsyncWebServer (Port 80)      │ • BMP280 Temperature Readings      │
│ • LittleFS Static File Server      │ • Orientation Classifier & Debounce│
│ • RESTful API Endpoints            │ • Physical Gesture Classifier      │
│ • RFC-5545 iCal Sync Client        │ • Pomodoro FSM & 1s Tick Loop      │
│ • OTA Firmware Updates             │ • TFT_eSPI Graphics Engine (25Hz)  │
│                                    │ • I2S Audio Synthesis & DMA Stream │
└────────────────────────────────────┴────────────────────────────────────┘
```

---

## 6. Software-in-the-Loop (SITL) Digital Twin

Kubi includes a complete desktop hardware simulation environment allowing 100% of firmware UI, gestures, audio, and state machine features to be tested and refined on a PC without touching physical silicon.

```
┌───────────────────────────┐         HTTP / JSON          ┌───────────────────────────┐
│     Desktop C++ Runner    │ <==========================> │   Web Simulation Studio   │
│   (LLVM-MinGW clang++)    │    GET /sim/frame (RGBA)     │  (React 19 + TypeScript)  │
│                           │    POST /sim/inject          │                           │
│ • Shared Firmware Source  │    GET /sim/state            │ • 3D Wooden Cube Chassis  │
│ • TFT_eSPI Mock Buffer    │    GET /api/state            │ • Interactive Rig Panel   │
│ • File NVS Preferences    │                              │ • Real-time Web Audio     │
└───────────────────────────┘                              └───────────────────────────┘
```

### Digital Twin Workbench (`http://localhost:5173/sim`)
* **3D Wooden Companion Cube**:
  - Handcrafted wood grain texture with 4 realistic corner brass machine screws.
  - Realistic glass glare reflection overlay.
  - **Auto-Orient Mode**: Chassis physically and smoothly rotates $-90^\circ$ into landscape orientation on Face 2 (Pomodoro) and Face 4 (Schedule).
  - Clicking the screen triggers a simulated gentle tap.
* **Hardware Rig Controls**:
  - Face orientation buttons (Face 1: Clock, Face 2: Pomodoro, Face 3: Mascot, Face 4: Schedule).
  - Gesture injection triggers (Gentle Tap, Shake to skip, Desk Slam ouch!).
  - Sliders for Temperature, Time Machine (fast-forwarding clock and timers), Battery, and 3-axis Accelerometer ($X, Y, Z$).
* **8-Bit Web Audio Synthesizer**: Recreates authentic square-wave tones in real-time based on firmware chime events.
* **Side-by-Side Companion Dashboard**: Full companion web app embedded in a synchronized tab.

---

## 7. Companion Web Dashboard

Kubi hosts a modern, responsive web companion served directly from onboard LittleFS flash memory at `http://kubi.local`.

* **Brand & Identity**: Document title rebranded to `Kubi` with custom amber box favicon (`box.svg`).
* **Interactive 3D Wireframe Cube**: Built using pure HTML5 2D Canvas vector projection with ambient glow, vertical sine bobbing, and dynamic floor shadow physics (zero heavy 3D library dependencies).
* **Live System Telemetry**: Active face mode, room temperature, battery level, Wi-Fi RSSI, and current Pomodoro status.
* **Pomodoro Controls**: Play, pause, skip, and reset actions; customizable focus, short break, and long break durations; customizable phase accent colors.
* **Schedule & Calendar Testing**: RFC-5545 `.ics` Google Calendar URL configuration, direct `.ics` file uploader, and quick sample calendar injector.
* **Companion Secret Override**: Send instantaneous priority broadcast banners across the cube's screen.

---

## 8. Development & Build Guide

### Prerequisites
* **Firmware**: [PlatformIO Core](https://platformio.org/install/cli) (`pio`)
* **Companion Dashboard**: [Node.js](https://nodejs.org/) (v18+) & `npm`
* **Desktop Simulator**: LLVM-MinGW (`clang++`) with C++17 support (automatically detected via WinGet on Windows)

### 1. Build and Flash ESP32 Firmware
```powershell
# Navigate to firmware directory
cd firmware

# Compile production binary (firmware.bin)
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run

# Build LittleFS filesystem image from firmware/data
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target buildfs

# Upload firmware and filesystem to connected ESP32
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target upload
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target uploadfs
```

### 2. Build Companion Web Dashboard
```powershell
cd dashboard

# Install dependencies
npm install

# Start local development server (proxies to http://localhost:8080 or http://kubi.local)
npm run dev

# Build production bundle directly into firmware/data/
npm run build
```

### 3. Run the Desktop SITL Simulator
```powershell
# Option A: From the dashboard directory
cd dashboard
npm run sim

# Option B: Run via PowerShell directly
powershell -ExecutionPolicy Bypass -File firmware/sim/run_sim.ps1
```
* **Simulator Workbench UI**: `http://localhost:5173/sim`
* **C++ Runner HTTP API**: `http://127.0.0.1:8080`

### 4. Rebuilding the Simulator Binary
```powershell
powershell -ExecutionPolicy Bypass -File firmware/sim/build_sim.ps1
```

---

## 9. Repository Structure

```
Kubi/
├── .agents/                    # Agent instructions & skills
│   └── skills/project-knowledge/ # Knowledge base skill definition
├── dashboard/                  # React 19 + TypeScript companion dashboard
│   ├── public/                 # Static web assets & box.svg favicon
│   ├── src/
│   │   ├── components/         # UI components (SpinningCube, Accordion, etc.)
│   │   ├── pages/              # Simulator studio & user manual
│   │   └── App.tsx             # Main companion dashboard application
│   ├── package.json
│   └── vite.config.ts          # Vite build config (outputs to firmware/data/)
├── docs/                       # Specifications, wiring diagrams, knowledge base
│   ├── Animation and Storage Optimization Guide.md # Memory math & diorama design
│   ├── Kubi Specifications.md  # Original product specifications
│   ├── Kubi Architecture and Wiring Guide.md # Hardware wiring blueprint
│   ├── PROJECT_KNOWLEDGE.md    # Comprehensive engineering knowledge base
│   └── knowledge/              # Modular, dated architecture & decision docs
│       └── index.md            # Knowledge catalog and index
├── firmware/                   # ESP32 C++ firmware & SITL simulation
│   ├── assets/sprites/         # GIF / PNG sprite animations
│   ├── data/                   # Compiled LittleFS dashboard assets
│   ├── include/                # Firmware headers
│   ├── partitions.csv          # Custom 2MB App / 1.8MB LittleFS partition table
│   ├── platformio.ini          # PlatformIO build configuration
│   ├── sim/                    # Software-in-the-Loop desktop simulator
│   │   ├── include/            # Desktop HAL mocks (Arduino, TFT_eSPI, Preferences)
│   │   ├── src/                # sim_main.cpp and TFT_eSPI_Mock.cpp
│   │   ├── build_sim.ps1       # Clang++ compilation script
│   │   └── run_sim.ps1         # Multi-process simulator launcher
│   ├── src/                    # Production firmware source code (shared 1:1)
│   │   ├── API.cpp / API.h
│   │   ├── AudioManager.cpp / AudioManager.h
│   │   ├── DisplayManager.cpp / DisplayManager.h
│   │   ├── KubiSprites.cpp / KubiSprites.h
│   │   ├── main.cpp
│   │   ├── PomodoroManager.cpp / PomodoroManager.h
│   │   ├── ScheduleManager.cpp / ScheduleManager.h
│   │   └── SensorManager.cpp / SensorManager.h
│   └── tools/
│       ├── build_sprites.py    # Pillow pre-build sprite slicer into PROGMEM
│       └── slice_spritesheet.py# Spritesheet slicing & animation generator
├── AGENTS.md                   # AI agent instructions & SITL quick reference
└── README.md                   # Project overview & documentation (this file)
```

---

## 10. Knowledge Base & Architectural References

Detailed design histories, state machine specifications, bug post-mortems, and hardware optimizations are maintained in [`docs/knowledge/`](docs/knowledge/index.md):

* [`2026-09-06: Diegetic Mascot Dioramas & Contextual Temperature Anchors`](docs/knowledge/2026-09-06-diegetic-mascot-dioramas-and-temperature-anchors.md)
* [`2026-09-06: Spritesheet Slicer Tool, Variable-Column Rows & Animation Pipeline`](docs/knowledge/2026-09-06-spritesheet-slicer-and-animation-pipeline.md)
* [`2026-09-06: Schedule Pagination & Gesture Navigation`](docs/knowledge/2026-09-06-schedule-pagination-and-gesture-navigation.md)
* [`2026-09-06: Embedded Finance Ticker & Dual-Side Clipping Masks`](docs/knowledge/2026-09-06-embedded-finance-ticker-and-clipping-masks.md)
* [`2026-09-06: Schedule Face RFC-5545 Calendar Parser & Empty States`](docs/knowledge/2026-09-06-schedule-face-calendar-parser-and-empty-states.md)
* [`2026-09-06: Dashboard Brand Migration & 3D Wireframe Cube`](docs/knowledge/2026-09-06-dashboard-brand-migration-and-3d-wireframe-cube.md)
* [`2026-09-06: High-Level Architecture, Heritage & System Structure`](docs/knowledge/2026-09-06-high-level-project-architecture-and-structure.md)
* [`2026-09-05: Digital Twin Workbench & Simulation Studio`](docs/knowledge/2026-09-05-digital-twin-workbench-and-simulation-studio.md)
* [`2026-09-05: Sensor Transients & False Tap Suppression`](docs/knowledge/2026-09-05-sensor-transients-and-tap-suppression.md)
* [`2026-09-05: Audio Chimes & Long Break Fanfare`](docs/knowledge/2026-09-05-audio-chimes-and-long-break-fanfare.md)
* [`2026-09-05: Pomodoro Idle State & Pause Persistence`](docs/knowledge/2026-09-05-pomodoro-idle-start-and-persistence.md)
* [`2026-09-05: Focus Clock Analog Toggle & NVS Persistence`](docs/knowledge/2026-09-05-focus-clock-analog-toggle-and-nvs-persistence.md)


---

<div align="center">
Crafted with precision for Wilhelm's 21st Birthday.
</div>
