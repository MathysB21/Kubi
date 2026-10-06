---
name: spritesheet-pipeline
description: >-
  Slice multi-row spritesheets into individual animations, generate GIFs and PNG sequences,
  and compile them into firmware PROGMEM byte arrays using slice_spritesheet.py and build_sprites.py
  for Project Kubi.
---

# Spritesheet & Animation Pipeline Skill

This skill governs the end-to-end asset workflow for Project Kubi: taking raw spritesheet images, slicing them into individual animations with variable columns, managing pixel art scaling, and compiling them into `PROGMEM` byte arrays for the ESP32 firmware and desktop SITL simulator.

---

## 1. Pipeline Overview

The Kubi animation pipeline consists of two automated stages:

```
[Raw Spritesheet Image]
         │
         ▼ (Stage 1: Slicing & Pre-Processing)
firmware/tools/slice_spritesheet.py
         │
         ├── Outputs .gif files or folders of .png frames
         ▼
firmware/assets/sprites/
         │
         ▼ (Stage 2: Automatic SCons / PlatformIO Build Hook)
firmware/tools/build_sprites.py
         │
         ├── Generates PROGMEM RGB565 byte arrays (0 RAM usage)
         ▼
firmware/include/KubiSprites.h
firmware/src/KubiSprites.cpp
         │
         ├── Consumed 1:1 by firmware & SITL simulator
         ▼
DisplayManager::drawMascotFace() / sim_main.cpp
```

---

## 2. Using `slice_spritesheet.py`

Location: [`firmware/tools/slice_spritesheet.py`](file:///d:/Developer/Kubi/firmware/tools/slice_spritesheet.py)  
Execution Python: Use PlatformIO's Python (`& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe"`) or any environment with Pillow installed.

### 2.1 Built-in Presets
When using known spritesheet layouts:
```powershell
# Slices the 4-tier Wilhelm Kubi sheet (Idle 4 cols, Walk 6 cols, Wave 4 cols, Desk 5 cols)
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/slice_spritesheet.py `
  -i "path/to/sheet.jpg" `
  --preset wilhelm
```

### 2.2 Custom Row Definitions
When rows have different column counts or specific pixel bounds:
Syntax for `-r / --rows`: `name:cols[:top_y:bottom_y]`

```powershell
# Even vertical division across 4 rows with different column counts:
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/slice_spritesheet.py `
  -i "sheet.png" `
  -r "kubi_idle:4,kubi_walk:6,kubi_wave:4,kubi_desk:5"

# Explicit pixel coordinates for irregular shelf heights:
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/slice_spritesheet.py `
  -i "sheet.png" `
  -r "kubi_idle:4:0:185,kubi_walk:6:185:375,kubi_wave:4:375:565,kubi_desk:5:565:764"
```

### 2.3 Pixel Art Scaling & Target Sizing
* **Nearest-Neighbor Integer Scale (`--scale <int>`)**: Multiplies pixels by $2\times$ or $3\times$ while keeping edges razor-sharp (no bilinear blurring).
* **Target Size (`-s WIDTHxHEIGHT`)**: Forces each frame to an exact canvas dimension (e.g. `-s 64x64`, `-s 96x96`).
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/slice_spritesheet.py `
  -i "sheet.png" `
  --preset wilhelm `
  --scale 2 `
  --format both
```

### 2.4 Output Formats (`--format`)
* `gif` (default): Emits looping `.gif` files into `firmware/assets/sprites/`.
* `png`: Emits a folder per animation containing numbered frames (`frame_00.png`, `frame_01.png`...).
* `both`: Generates both the `.gif` and the frame folder.

---

## 3. Firmware Compilation (`build_sprites.py`)

Location: [`firmware/tools/build_sprites.py`](file:///d:/Developer/Kubi/firmware/tools/build_sprites.py)

* **Automatic Execution**: SCons runs this automatically before every `pio run` build.
* **Manual Execution**:
  ```powershell
  & "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/build_sprites.py
  ```
* **Output Structure**:
  ```cpp
  // Auto-generated in firmware/include/KubiSprites.h
  struct KubiSpriteAnimation {
      const char* name;
      uint16_t width;
      uint16_t height;
      uint16_t frameCount;
      uint16_t frameDelayMs;
      const uint16_t* const* frames;
  };

  extern const KubiSpriteAnimation anim_kubi_idle;
  extern const KubiSpriteAnimation* const ALL_KUBI_ANIMATIONS[];
  extern const size_t ALL_KUBI_ANIMATION_COUNT;
  ```

---

## 4. Memory Budget & Best Practices

1. **Flash Cost (2 Bytes/Pixel)**:
   - Frame size formula: $\text{Bytes} = \text{width} \times \text{height} \times 2$.
   - A $48 \times 48$ frame = $4,608$ bytes ($4.5\text{ KB}$).
   - A $64 \times 64$ frame = $8,192$ bytes ($8.0\text{ KB}$).
   - A $240 \times 320$ full-screen frame = $153,600$ bytes ($150\text{ KB}$).
2. **Current Flash Headroom**:
   - `app0` partition has **~714 KB free**.
   - Suitable for ~15–35 character animations ($48\times 48$ to $64\times 64$) or ~4–6 multi-frame set-piece scenes.
3. **Zero Dynamic RAM Consumption**:
   - All arrays are marked `PROGMEM` and live in flash memory mapped to CPU cache.
   - Adding animations does not deplete ESP32 heap.
4. **Full-Screen Scene Optimization**:
   - For full-screen dioramas ($240\times 320$), prefer **1 static full-screen background frame** paired with a smaller animated character sprite moving on top, rather than duplicating the entire $240\times 320$ room on every single frame.

---

## 5. Verification Checklist

When adding new animations or modifying sheets:
1. Slices generated without errors in `firmware/assets/sprites/`.
2. Run `build_sprites.py` or `pio run` to ensure `KubiSprites.cpp` compiles cleanly.
3. Verify memory usage in PlatformIO build output (`pio run` must report $<100\%$ Flash).
4. Run SITL desktop simulator or test build to verify frame rendering on display.
