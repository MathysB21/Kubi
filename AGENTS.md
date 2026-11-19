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
  - `POST /sim/inject`: Injects virtual face changes (`{"face": 1}`; 0-3 upright, 4 = screen up, the maze), gestures (`{"gesture": "tap"|"shake"|"slam"}`), continuous tilt (`{"tiltRoll": deg, "tiltPitch": deg}`, relative to the current face resting flat), the first-connect screen (`{"showAddress": true}`), the Face 3 scene (`{"mascotScene": n}`), temp, battery, accel.
  - `GET /sim/state`: Telemetry heartbeat (orientation, backlight PWM, chime events, active face, tilt, `audioSamples`/`audioUnderruns`, `pixelWrites`, and `maze` while the maze is up).
  - `GET /api/state`: Standard Kubi firmware REST state (incl. `sleepTimeoutMin`, `isSleeping`, `mascotScene`, `mascotPreview`, `birthday`, `volume` 0-100, `battery` %, `batteryVoltage`, `batteryState` draining/charging/full/disconnected (cells cut off by the charger's protection)/checking (just started, not yet sure about the charger), `battery` -1 for the first seconds, `batteryMinutes` to empty or full (-1 = estimating), `pressure` hPa, `chargeAlertHigh`/`chargeAlertLow`).
  - `POST /api/settings`: Updates Pomodoro intervals/colours, clock style, `sleepTimeoutMin`, `mascotScene` (a 10-minute preview, or `mascotPreviewMin` up to 720 from the Creator panel; -1 hands back to Kubi's pick; `/api/state` has `mascotPreviewLeftMin` while one shows), `birthday` ("MM-DD", NVS `bday`), `chargeAlertHigh`/`chargeAlertLow` (0 = off, NVS `alertHi`/`alertLo`; from 22:00 to 07:00 they show the banner silently, no chime or wake), `volume` (0-100, NVS `volume`, plays a tap chime to confirm), face.
  - `POST /api/override`: `{"title"?, "message"}` puts a banner up (wrapped to 3 lines; no title = "! INCOMING ALERT !"); an empty message takes it down.
  - `POST /api/chime`: `{"chime": "tap"|"done"|"fanfare"|"ping"|"ouch"}` plays any chime.
  - (Hardware only, not in the sim) `POST /api/motion-log` `{"seconds": 1-30}` records every accelerometer sample with what the gesture detectors made of it; `GET /api/motion-log` returns `{"state": 0|1}` or, when done, the CSV. `GET /api/diagnostics` includes `resetReason` (poweron, software, panic, brownout, task_wdt, ...).
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
   - **Hardware Rig Tab**: Orientation buttons (Face 1: Clock, Face 2: Pomodoro, Face 3: Mascot, Face 4: Ambient, Screen UP: Maze), Gesture triggers (Gentle Tap, Shake to skip, Desk Slam ouch!), sliders for Temperature, Time Machine, Battery, and 3-axis Accelerometer.
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
  - `firmware/src/KubiSprites.cpp`, `firmware/src/KubiScenes.cpp` (both generated and committed; the pre-build scripts regenerate them when Pillow is installed in PlatformIO's Python and skip otherwise)
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
4. **Taps, and Not Mistaking Handling for Them** (tuned on the assembled cube 2026-11-17; the reasoning and numbers are at the top of `SensorManager.cpp`):
   - The IMU sits on padded standoffs, so taps arrive soft: a deltaMag of 2.4-5 for soft taps, up to ~28 for the hardest. Putting the cube down or rolling it peaks at 2-12 as well, so size alone cannot tell them apart. Stillness can: a tap is a spike >= `TAP_MIN_DELTA` (2.2) within `TAP_ONSET_MS` of a burst that began after `TAP_STILL_MS` (500 ms) below `STILL_DELTA` (0.4). Every recorded tap had <= 0.16 before it, every handling spike >= 0.63.
   - A tap is confirmed after `TAP_CONFIRM_MS` (250 ms) once nothing above `MOTION_WAKE_DELTA` has moved for `TAP_SETTLE_MS`; still moving after `TAP_MAX_RING_MS` means a grab, and it is dropped. Shakes cancel it (a reversal later than `TAP_REBOUND_MS`, so a tap's own rebound does not), and one bout of shaking reports one shake (`SHAKE_QUIET_MS`).
   - In `SensorManager::updateFace()`, a pose flip blocks new tap candidates (`_lastTapTime`), but a pending tap is only dropped once another pose has held `POSE_MOVE_MS` (100 ms): a hard tap knocks gravity off its axis for one sample, and dropping on the flip threw away exactly the firm taps. Drop taps only, never shakes. `main.cpp` drains any pending gesture when the face actually switches.
   - Slam ("ouch") is |reading| > `SLAM_TOTAL` (50); hard taps reached 26.6 and a shake 26.8. Samples under `MIN_VALID_MAG` (failed I2C reads read ~0 g) are discarded.
   - **Replay before and after any change**: `powershell -ExecutionPolicy Bypass -File firmware/sim/replay_motion.ps1` runs the real `SensorManager` over the recordings in `firmware/sim/motion/` (expected results in its README). Record new ones on the cube with `POST /api/motion-log {"seconds": 30}` then `GET /api/motion-log` (CSV).
   - Shakes are sign reversals of the reading minus a slow gravity estimate (`GRAVITY_ALPHA`), on all three axes. Never threshold a raw axis: upright, gravity sits on X or Y depending on the face, so a raw-axis detector only works on some faces. Test gestures with the cube upright on each face, not lying flat. Replay real motion through `SensorManager` when changing any of this; the sim's gesture buttons inject gestures directly and skip all of it.
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
  - Ten full-screen diorama scenes (`KubiScenes.cpp`, generated by `tools/build_scenes.py` from `art/1K/` as 120x160 RLE drawn at 2x, a 64-colour palette per scene weighted towards Kubi's blue) with a temperature badge from the BMP280. Falls back to the vector jelly mascot if no scenes are compiled in.
  - Kubi picks the scene himself from the date and time (`SceneSchedule.h`); tap or shake only makes him blink. The dashboard can preview one for 10 minutes (`mascotScene`). Birthday (on NVS `bday`, Wilhelm's: 11-16) and Christmas (1-26 Dec) are Easter eggs: never list them in the dashboard. The one exception is the hidden Creator panel (`dashboard/src/components/CreatorPanel.tsx`, the brother's backdoor: tap the KUBI title 5 times within 3 s, remembered per browser in localStorage), which can show any scene for 10 min to 12 h, put up a titled banner, play any chime, and has a one-tap Birthday surprise (Face 3 + Birthday scene + fanfare + banner). It saves nothing on Kubi. A held scene keeps the screen awake.
  - Light animation per scene (`SCENE_ANIM` in `build_scenes.py`: blink, glints, particles, ripple, wiggle), drawn by `DisplayManager::animateScene` over a decoded 19 KB copy of the scene, touching only the pixels that change. `KUBI_SCENE_PREVIEW=<dir>` makes the script write placement previews.
  - The panel is BGR (`-D TFT_RGB_ORDER=0`): without it every red showed blue, Kubi orange, and the gold/orange UI cyan.
  - A scene is a whole screen of SPI: it is painted only when the scene changes, and the badge only when the reading changes.
  - The only face (with the maze) that sleeps: after `sleepTimeoutMin` of stillness (default 5, NVS `sleepMin`, dashboard "Screen Sleep") the backlight fades out; any tap or movement wakes it silently.
* **Face 4: Ambient Glow** (`AmbientFace.cpp`, always; the maze moved to screen-up on 2026-11-17):
  - Dithered pixel-art orb (8 px cells, 4x4 Bayer fringe) that breathes (9 s) and eases through a night palette (40 s per colour).
  - Backlight fades down to `AmbientFace::BACKLIGHT` on this face. Exempt from screen sleep. Tap moves on to the next colour, silently.
  - Redraws at 10 Hz and pushes only cells whose colour changed (~8k px/s).
* **Screen up: Tilt Maze** (`MazeGame.cpp`, `MazeBoards.cpp`; `MODE_MAZE` = 4, `FACE_SCREEN_UP` in `FaceMap.h`). Lay Kubi on his back, screen to the sky, and tilt it like a marble maze. Screen down is still no face (the last face stays). The old `face4Maze` setting is gone.
  - Drawn in the rotation of the last upright face (`mazeViewFace`), so it reads right for whoever tipped him back. Portrait views (Faces 1 and 3) show the board transposed (14 wide, 20 tall, 15 px cells) with a shorter HUD; landscape views show it as drawn.
  - Ten 20x14 ASCII boards (`#` wall, `S`, `G`, `O` hole). Steering is real gravity: the reading minus `SCREEN_UP_LEVEL_*` (`FaceMap.h`, the level reading measured in the cube 2026-11-18 by averaging flat and turned 180 degrees). Never capture a "rest" at board start: the maze appears while the cube is still being tipped back, and that snapshot made a level cube steer. Entering the maze waits (ball on S) until the cube has been still for 400 ms. Right edge down rolls right, top edge down rolls up, in the view face's screen frame (`tiltedGravityFlat` models it for the sim). Fixed 200 Hz physics in cell units, circle-vs-cell collisions, no tunnelling at the speed cap.
  - While playing, faces need a 2 s hold to switch (stand him up and hold to leave); the lock releases after 45 s without input.
  - Holes send the ball back to S (clock keeps running), the goal shows time + best (NVS `mzb<n>`), then the next board. Renders at 30 fps, ball only.

---

## 5b. Architecture Notes (read before changing these areas)

* **Audio** runs in its own FreeRTOS task (`audioTask` in `main.cpp`, a thread in the sim) that blocks on the I2S DMA queue. `playChime()` is safe from any task. The sim's `AudioOutputI2S` mock is deliberately strict: no samples before `begin()`, a DMA-sized queue drained at the sample rate, underruns counted.
* **Audio hardware**: the build flag `KUBI_AUDIO_INTERNAL_DAC` (on, in `platformio.ini`) sends audio out of the ESP32's 8-bit DAC on GPIO 25/26 into a PAM8403 analog amp, because no MAX98357A was obtainable. The DAC rests at 0 V (what the DMA writes on underrun) and a chime is a pulse wave from 0 V up, never a square wave around a raised level. Both alternatives failed on the real cube (2026-11-11): fading to mid-scale around each chime kicked the speakers through the amp hard enough to brown out the ESP32 at WiFi start-up, and holding a raised level between chimes dropped to 0 V whenever a flash write (OTA, NVS, LittleFS) stalled the audio task, which with the knob up tripped the charger's battery protection; a raised level also passes the 3.3 V rail's noise (the DAC divides that rail) as audible static. Volume is scaled in the synth there (`DAC_VOLUME_SCALE`), the library gain stays at 1.0. Without the flag the original MAX98357A I2S path is unchanged. The mock records DAC codes (`getDacTrace()`) when built with the flag.
* **Rendering** is scene-based (`DisplayManager::beginScene`). Only a scene change, rotation or `invalidate()` clears the screen; faces repaint just what changed. Never add `fillScreen` to a steady-state draw path. The TFT mock implements TFT_eSPI's opaque text background and `setTextPadding`, so overdraw bugs show in the sim.
* **Screen sleep** policy lives in `DisplayManager` (`noteActivity`, `setSleepAllowed`, `requestWake` for other tasks); which faces sleep is `modeMaySleep()` in `API.h`.
* **Orientation** has one source of truth: `FACE_POSES` and `SCREEN_OUT_*` in `FaceMap.h`, measured in the assembled cube 2026-11-17 (faces on +Y, -X, -Y, +X; screen out -Z; the breadboard had +X, -Y, -X, +Y and +Z). Sensors, rotation, the sim's face buttons and tilt injection, and maze steering all read it. Rerun the script, never hand-edit the rows, if the sensor moves relative to the screen.
* **Cross-task data**: never share an Arduino `String` between the web server task and the hardware loop; see `setOverrideText()/getOverrideText()`. Web handlers must not touch the TFT: they set state and the hardware loop applies it (e.g. rotation follows `currentMode`).
* **OTA callbacks** (`setupOTA()` in `main.cpp`) run in `loop()`'s task. They set `otaInProgress` and nothing else; the hardware task draws the update screen. Touching the TFT from a second task corrupts TFT_eSPI's shared SPI lock and asserts in FreeRTOS.
* **Crash debugging**: a panic writes a core dump to the `coredump` partition (`0x3F0000`, ELF format) and the next boot logs `[CRASH] A core dump ... is stored`. Read and decode it over USB with `powershell -ExecutionPolicy Bypass -File firmware/tools/read_coredump.ps1 -Port COMx`. The ELF must be the build that crashed: if the tree has moved on, rebuild that commit in a worktree and pass `-Elf`. For a one-off backtrace from the serial log, `xtensa-esp32-elf-addr2line -pfiaC -e firmware.elf <addresses>` against the same ELF.
* **Battery and power path**: the MT3608 (~5.05 V) and the DevKit's USB (through its diode, ~4.7 V) both feed the 5 V rail, so the cells carry the whole load even with the ESP32's USB plugged in; that USB only takes over once the cells are cut off, and it never charges them (only the TP4056's USB-C does). `BatteryMonitor` (`Battery.h`) reports `disconnected` below 2 V (the protection has cut the cells off), seeds itself over a short warm-up (the charger's own 4.2 V shows for a moment at power-on), and is not `sure()` whether it is charging for up to 10 minutes after a start. **Low-battery shutdown** (`enterBatterySleep` in `main.cpp`): draining, sure, average under `BATTERY_EMPTY_V` (3.30 V) for a minute, then a message and deep sleep, waking every 60 s to read the cell before anything else in `setup()` and starting only on a 0.05 V rise (charging) or above 3.75 V. Picking him up does not wake him. Added 2026-11-19 after the pack ran down to the protection's 2.5 V overnight. Hardware only, like the nap; the sim's battery is a slider.
* **CPU clock** is fixed at 160 MHz from boot (`board_build.f_cpu` in `platformio.ini`, for battery; `/api/diagnostics` has `cpuMhz`). Never call `setCpuFrequencyMhz()` at runtime: switching 240 <-> 80 on the cube (2026-11-18) retunes the PLL the WiFi radio shares and knocked WiFi and OTA out until a reset.
* **When OTA fails**: "No response from device" with Kubi answering HTTP is usually this PC's firewall (the WiFi network profile was Public; espota needs inbound UDP/TCP). An upload dying part-way usually just works on a retry. If Kubi is off the network, flash over USB (the ESP32's micro-USB, CH340 on COMx). The cube's board does not auto-enter download mode ("Wrong boot mode 0x13"): have the user hold BOOT, tap EN, release BOOT, then run esptool without a reset: `python esptool.py --chip esp32 --port COMx --baud 460800 --before no_reset --after hard_reset write_flash -z 0xe000 <arduino>/tools/partitions/boot_app0.bin 0x10000 .pio/build/esp32dev_usb/firmware.bin` (`pio -t upload` resets it out of download mode first). A failed USB attempt still resets the board, which is a way to restart it.
* **Changing `partitions.csv`** only takes effect over USB (`-e esp32dev_usb -t upload`); OTA never rewrites the table. If the `spiffs` partition moves or resizes, run `uploadfs` as well or the dashboard is gone.
* **Physical build**: `enclosure/kubi_enclosure.scad` is the parametric case (README there for printing); `docs/Kubi Architecture and Wiring Guide.md` sections 4-5 and `docs/Kubi Assembly Guide.md` are the wiring and assembly. `viewer/` (React + Vite + three.js, `npm run dev`, port 5174) shows both in 3D. It is a separate app on purpose: three.js would not fit the cube's 896 KB LittleFS. Its data in `viewer/src/model/` mirrors the guide and the SCAD file; change them together, and re-export `enclosure/stl/assembly/` (the `*_asm` parts) when the enclosure changes.
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

