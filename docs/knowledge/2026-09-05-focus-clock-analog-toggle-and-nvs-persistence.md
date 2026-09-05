# 2026-09-05: Focus Clock Analog Toggle and NVS Persistence

## 1. Context & Motivation
Face 1 on Kubi serves as the primary desk companion clock. Initially, Face 1 only displayed digital time in large 7-segment font and had a legacy tap detail overlay (showing calendar events and stock tickers) that could be accidentally triggered during orientation changes.
The requirements evolved to:
1. Add an integrated date display (`Thu, 3 Sep` / `Sat, 8 Aug`) without altering the centered time position.
2. Provide an analog clock face option accessible via a physical shake gesture.
3. Remove the legacy tap overlay to maintain a clean, distraction-free minimalist aesthetic.
4. Persist the user's clock view choice (digital vs. analog) across face switches, reboot, and desktop simulator sessions.

## 2. Key Architecture & Design Decisions

### Date Display Layout
- In [`DisplayManager::drawClockFace()`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp):
  - Digital time remains strictly centered (`MC_DATUM` at `(cx, cy)`).
  - Date string is formatted as `%s, %d %s` (e.g. `Thu, 3 Sep`, `Sat, 8 Aug`) using standard abbreviations for days and months.
  - Drawn 40px below center in Font 2 (approximately 1/4 the height of the 7-segment font).

### Analog Clock View
- In [`DisplayManager::drawAnalogClockFace()`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp):
  - **Dial**: 12 radial hour numbers rendered in Font 2 at radius $r = 82\text{px}$ from center `(cx, cy)`.
  - **Bezel**: Borderless design with no circular outer ring for a clean, floating dial look.
  - **Hour Hand**: 3px wide, orange (`TFT_ORANGE`), length $r = 44\text{px}$, calculated with continuous sub-hour angle resolution ($\text{hour} + \frac{\text{minute}}{60}$).
  - **Minute Hand**: 2px wide, white (`TFT_WHITE`), length $r = 66\text{px}$.
  - **Center Pivot**: Orange concentric circle ($r = 3\text{px}$) with white center pip ($r = 1\text{px}$).
  - **Date**: Rendered at bottom `(cx, cy + 124)` in `TFT_LIGHTGREY`.

### Shake Toggle & NVS Persistence
- **Hardware (`main.cpp`)**:
  - `GESTURE_SHAKE` when `currentMode == MODE_CLOCK_IDLE` inverts `clockAnalogView`.
  - Saves the boolean to ESP32 Non-Volatile Storage (NVS) using `preferences.begin("kubi_settings", false)` under key `"clockAnalog"`.
  - On startup in `setup()`, `clockAnalogView = preferences.getBool("clockAnalog", false)` restores the saved preference.
- **REST API (`API.cpp`)**:
  - `GET /api/state` reports `doc["clockAnalog"] = clockAnalogView`.
  - `POST /api/settings` accepts `"clockAnalog": boolean` and saves to NVS.
- **Desktop Simulation HAL (`Preferences.h` & `sim_main.cpp`)**:
  - `Preferences.h` mock maintains key-value mappings and synchronizes them to disk (`kubi_sim_prefs.txt`) upon `put*()` / `end()`.
  - `sim_main.cpp` loads `clockAnalogView` at startup and persists it on shake or API settings update.

## 3. Code Touchpoints & Files
- [`firmware/src/DisplayManager.h`](file:///c:/Development/Kubi/firmware/src/DisplayManager.h): Added `drawAnalogClockFace()` declaration and updated `drawClockFace()` signatures.
- [`firmware/src/DisplayManager.cpp`](file:///c:/Development/Kubi/firmware/src/DisplayManager.cpp): Implemented date formatting, analog clock trigonometry/rasterization, and removed legacy tap overlay.
- [`firmware/src/main.cpp`](file:///c:/Development/Kubi/firmware/src/main.cpp): Added `clockAnalogView` global, shake gesture handler with NVS save, and boot preference loading.
- [`firmware/src/API.cpp`](file:///c:/Development/Kubi/firmware/src/API.cpp): Exposed `clockAnalog` in `/api/state` and `/api/settings`.
- [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Parity for shake toggle, persistence loading/saving, and API endpoints.
- [`firmware/sim/include/Preferences.h`](file:///c:/Development/Kubi/firmware/sim/include/Preferences.h): Added `getBool()`, `putBool()`, and disk file persistence (`kubi_sim_prefs.txt`).

## 4. Gotchas & Verification
- **Gotcha**: When drawing thick angled clock hands, offset coordinates by normal unit vectors $(\pm n_x, \pm n_y)$ perpendicular to hand angle rather than simple pixel offsets to avoid distorted lines at diagonal angles.
- **Gotcha**: Windows locks active binaries. When testing desktop simulator builds, output to a temporary binary (e.g. `kubi_sim_test.exe`) so the user's running `kubi_sim.exe` process is not interrupted.
- **Verification**:
  1. Trigger shake on Face 1 (in simulator UI or hardware) $\rightarrow$ clock transitions between digital and analog with feedback chime.
  2. Rotate cube to Face 2 (Pomodoro) and back to Face 1 $\rightarrow$ chosen view is retained.
  3. Reboot ESP32 or restart simulator runner $\rightarrow$ chosen view is loaded from NVS/disk.
