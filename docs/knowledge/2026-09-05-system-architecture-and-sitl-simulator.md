# 2026-09-05: System Architecture and Software-in-the-Loop (SITL) Simulator

## 1. Context & Motivation
Project Kubi runs on an ESP32 dual-core SoC with a 240×320 IPS display, I2C motion/environmental sensors, and I2S audio.
To allow rapid iteration and debugging without physical hardware connected, the project includes a Software-in-the-Loop (SITL) digital twin simulator.

## 2. Key Architecture & Design Decisions

### Zero Code Duplication
Core firmware modules are shared 1:1 between real silicon and desktop simulation:
- `PomodoroManager.cpp`, `DisplayManager.cpp`, `SensorManager.cpp`, `AudioManager.cpp`, `KubiSprites.cpp`.
- Hardware-specific peripherals are mocked in `firmware/sim/include/` (`TFT_eSPI_Mock.cpp`, `Preferences.h`, `Arduino.h`, `AudioOutputI2S.h`).

### Dual-Core Threading Model
- **Core 0**: Wi-Fi, `ESPAsyncWebServer`, LittleFS dashboard static assets, iCal calendar synchronization, REST API endpoints.
- **Core 1**: Hardware sensor polling (50–60Hz), display rendering (20–25Hz), I2S audio synthesis, Pomodoro countdown.

### Desktop Simulator Runner Protocols
- User runs `kubi_sim.exe` externally in a dedicated terminal window.
- **CRITICAL PROTOCOL**: Agents must **NOT** launch or kill `kubi_sim.exe`.
- Because Windows locks running `.exe` files, compiling with `build_sim.ps1` while the simulator is running will fail with file lock errors. When testing simulator builds with `clang++`, compile to a temporary test binary (`kubi_sim_test.exe`).

## 3. Code Touchpoints & Files
- [`firmware/sim/src/sim_main.cpp`](file:///c:/Development/Kubi/firmware/sim/src/sim_main.cpp): Desktop simulation runner and HTTP API.
- [`firmware/sim/src/TFT_eSPI_Mock.cpp`](file:///c:/Development/Kubi/firmware/sim/src/TFT_eSPI_Mock.cpp): Framebuffer rasterizer.
- [`firmware/sim/include/Preferences.h`](file:///c:/Development/Kubi/firmware/sim/include/Preferences.h): Desktop NVS Preferences emulation with file persistence (`kubi_sim_prefs.txt`).
- [`dashboard/src/Simulator.tsx`](file:///c:/Development/Kubi/dashboard/src/Simulator.tsx): Web-based digital twin workbench.

## 4. Build & Verification Commands
- Real Silicon: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run`
- Dashboard: `npm run build` in `dashboard/`
- Simulator Clang Test:
  ```powershell
  $clang = (Get-ChildItem "C:\Users\Mathys\AppData\Local\Microsoft\WinGet\Packages" -Recurse -Filter "clang++.exe" | Select-Object -First 1).FullName
  & $clang -std=c++17 -O2 -static -I firmware/sim/include -I firmware/include -I firmware/src -I firmware/.pio/libdeps/esp32dev/ArduinoJson/src firmware/src/PomodoroManager.cpp firmware/src/DisplayManager.cpp firmware/src/SensorManager.cpp firmware/src/AudioManager.cpp firmware/src/KubiSprites.cpp firmware/sim/src/TFT_eSPI_Mock.cpp firmware/sim/src/sim_main.cpp -lws2_32 -o firmware/sim/kubi_sim_test.exe
  ```
