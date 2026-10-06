# 2026-09-06: Spritesheet Slicer Tool, Variable-Column Rows, and Animation Pipeline

## 1. Context & Motivation
In Project Kubi, character animations for Face 3 (Mascot Face) were previously limited to a single test bouncing jelly sprite (`kubi_bounce.gif`). The user provided a multi-row pixel art spritesheet featuring Kubi in various states (idle bounce, walking, waving, and working at a desk).

However, real-world spritesheets frequently have **differing numbers of columns per row** (e.g. Row 1 has 4 frames, Row 2 has 6 frames, Row 3 has 4 frames, Row 4 has 5 frames) and irregular vertical shelf heights. Manually cropping and extracting these frames in an image editor is tedious and error-prone. A dedicated, automated CLI tool was needed to slice multi-tier sheets into looping GIFs or PNG sequences directly compatible with Kubi's automated PlatformIO pre-build pipeline (`build_sprites.py`).

## 2. Key Architecture & Design Decisions

### 2.1 Multi-Row & Variable-Column Slicing Architecture
The tool [`firmware/tools/slice_spritesheet.py`](file:///d:/Developer/Kubi/firmware/tools/slice_spritesheet.py) was implemented with:
- **Flexible Row Parsing (`--rows`)**: Accepts comma-separated row specifications with independent column counts and optional pixel bounds:
  `name:cols[:top:bottom]` (e.g. `"idle:4:0:185,walk:6:185:375"`).
- **Built-in Presets (`--preset`)**: Ships with a ready-to-use `--preset wilhelm` mapped directly to the uploaded 1024×764 sheet:
  - Row 1: `kubi_idle` (4 cols, $y \in [0, 185]$, 150ms delay)
  - Row 2: `kubi_walk` (6 cols, $y \in [185, 375]$, 120ms delay)
  - Row 3: `kubi_wave` (4 cols, $y \in [375, 565]$, 150ms delay)
  - Row 4: `kubi_desk` (5 cols, $y \in [565, 764]$, 180ms delay)
- **Direct Pipeline Integration**: Outputs directly into [`firmware/assets/sprites/`](file:///d:/Developer/Kubi/firmware/assets/sprites/), where the SCons pre-build hook [`build_sprites.py`](file:///d:/Developer/Kubi/firmware/tools/build_sprites.py) automatically ingests them and generates C++ `PROGMEM` RGB565 arrays.

### 2.2 Nearest-Neighbor Integer Scaling (`--scale` & `--target-size`)
To prevent muddy bilinear interpolation artifacts on retro pixel art, the slicer implements nearest-neighbor scaling (`Image.NEAREST`):
- Passing `--scale 2` converts $48\times 48$ frames into razor-sharp $96\times 96$ frames ($40\%$ of the $240\text{px}$ screen width).
- Passing `-s 64x64` enforces exact bounding box dimensions.

### 2.3 Multiple Export Formats
- `gif`: Animated looping GIF directly picked up by `build_sprites.py`.
- `png`: Numbered frame sequence (`frame_00.png`, `frame_01.png`...) inside an animation subfolder.
- `both`: Emits both formats simultaneously for inspection and backup.

## 3. Code Touchpoints & Files
- [`firmware/tools/slice_spritesheet.py`](file:///d:/Developer/Kubi/firmware/tools/slice_spritesheet.py): CLI tool for slicing multi-tier spritesheets with variable column counts, presets, and scaling.
- [`firmware/tools/build_sprites.py`](file:///d:/Developer/Kubi/firmware/tools/build_sprites.py): PlatformIO pre-build script that converts `.gif` and `.png` frame folders into `KubiSprites.h` and `KubiSprites.cpp`.
- [`.agents/skills/spritesheet-pipeline/SKILL.md`](file:///d:/Developer/Kubi/.agents/skills/spritesheet-pipeline/SKILL.md): Comprehensive agent skill defining slicing syntax, build hooks, and memory optimization guidelines.

## 4. Gotchas & Verification
1. **Python Environment on Windows**:
   - Always invoke the tool using PlatformIO's Python environment where Pillow is installed:
     ```powershell
     & "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/slice_spritesheet.py -i "sheet.png" --preset wilhelm
     ```
2. **Flash Budget Awareness**:
   - Frames in `PROGMEM` take 2 bytes per pixel ($W \times H \times 2$).
   - Current `app0` headroom is **~714 KB**. Slicing large frames (e.g. $>150\times 150$) across many frames will rapidly fill the flash partition. Keep character sprites between $48\times 48$ and $80\times 80$.
3. **Verification Command**:
   - Run slice with `--format both` to a test directory, inspect the generated GIF in a viewer, and verify PlatformIO build cleanly compiles the resulting sprites:
     ```powershell
     & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
     ```
