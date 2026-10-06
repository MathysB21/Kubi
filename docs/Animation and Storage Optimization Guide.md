# Animation & Storage Optimization Guide

This reference guide details the memory architecture, pixel mathematics, storage budgets, and optimization strategies for running animations and graphics on **Project Kubi** (ESP32-WROOM-32).

---

## 1. Hardware Memory Specifications & Current Allocation

The ESP32-WROOM-32 features:
* **4 MB (4,096 KB) High-Speed SPI Flash**
* **320 KB Internal SRAM** (Dynamic Heap & Stack)
* **240 MHz Dual-Core Xtensa LX6 CPU**

### Partition Layout (`firmware/partitions.csv`)
| Partition | Type | SubType | Flash Offset | Allocated Size | Current Usage | Free Headroom |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`nvs`** | Data | NVS | `0x9000` | 20 KB (`0x5000`) | < 4 KB | ~16 KB |
| **`otadata`** | Data | OTA | `0xe000` | 8 KB (`0x2000`) | - | - |
| **`app0`** | App | OTA_0 | `0x10000` | **2,048 KB (2.0 MB)** | **~1,383 KB (66.0%)** | **~714 KB (34.0%)** |
| **`spiffs` (LittleFS)** | Data | SPIFFS | `0x210000` | **1,920 KB (1.875 MB)** | **~556 KB (28.9%)** | **~1,364 KB (1.36 MB)** |

### Dynamic RAM (SRAM Heap)
* **Total SRAM**: 320 KB
* **Currently Used**: ~56 KB (17.2%)
* **Free Dynamic RAM**: **~271 KB (82.8% free)**

> [!IMPORTANT]
> **The Zero-RAM Principle for Graphics (`PROGMEM`)**:  
> In ESP32 C++, graphics and sprite arrays declared with `PROGMEM` (e.g. `static const uint16_t frame[] PROGMEM = { ... }`) are mapped directly into the CPU's Flash Cache memory space (DROM/IROM).  
> **They consume 0 bytes of dynamic RAM heap.** You can store 20, 40, or 60 animation frames in flash without reducing the ESP32's free RAM by a single byte.

---

## 2. Pixel Mathematics & Raw RGB565 Cost

The ST7789 display controller operates in **16-bit RGB565 color**:
* **Bytes per pixel**: $2\text{ bytes}$ (5 bits Red, 6 bits Green, 5 bits Blue).
* **Storage formula per frame**:  
  $$\text{Bytes} = \text{Width} \times \text{Height} \times 2$$

### Resolution vs. Memory Footprint
| Sprite / Frame Dimensions | Pixels | Bytes per Frame | 4-Frame Animation | 6-Frame Animation | How Many Fit in Current 714 KB `app0` Headroom? |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **$40 \times 40\text{ px}$** (Compact) | 1,600 | **3.2 KB** | 12.8 KB | 19.2 KB | **~55 animations** (~220 frames) |
| **$48 \times 48\text{ px}$** (Standard) | 2,304 | **4.6 KB** | 18.4 KB | 27.6 KB | **~38 animations** (~155 frames) |
| **$64 \times 64\text{ px}$** (High-Detail)| 4,096 | **8.2 KB** | 32.8 KB | 49.2 KB | **~21 animations** (~85 frames) |
| **$80 \times 80\text{ px}$** (Large Mascot)| 6,400 | **12.8 KB** | 51.2 KB | 76.8 KB | **~14 animations** (~55 frames) |
| **$160 \times 90\text{ px}$** (Desk Scene)| 14,400 | **28.8 KB** | 115.2 KB | 172.8 KB | **~4 to 6 scenes** (~24 frames) |
| **$240 \times 320\text{ px}$** (Full Screen)| 76,800 | **153.6 KB** | 614.4 KB | 921.6 KB | **~1 animation** (4 frames max) |

---

## 3. Full-Screen Scenes: The Layered Diorama Strategy

On Face 3 (Mascot Face), the display is $240\times 320$ in portrait mode.

### The Problem with Full-Frame Animations
If a 5-frame animation re-renders every single pixel of the entire $240\times 320$ screen on each frame:
$$5 \times 153.6\text{ KB} = 768\text{ KB}$$
A single animation would immediately exhaust the entire 714 KB free firmware flash budget!

### The Solution: Layered Scene Composition
In retro game engines (Game Boy, SNES, arcade machines), background environments remain static while only the character sprite and interface elements animate:

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Static Full-Screen Background ($240 \times 320$)         │  153.6 KB (1 frame)
│    (Walls, bookshelf, desk, lamp, wooden calendar block)    │
├─────────────────────────────────────────────────────────────┤
│ 2. Animated Mascot Sprite ($64 \times 64$ or $80 \times 80$)│   19.2 KB (6 frames)
│    (Kubi typing on laptop at position X: 80, Y: 140)        │
├─────────────────────────────────────────────────────────────┤
│ 3. Diegetic UI Temperature Overlay                          │    0.0 KB (Text render)
│    (Live room temperature drawn onto the wooden block)      │
├─────────────────────────────────────────────────────────────┤
│ TOTAL MEMORY FOR COMPLETE LIVING ROOM:                      │ ~172.8 KB
└─────────────────────────────────────────────────────────────┘
```

**Result**: Instead of 1 full-screen animation taking all available flash, **3 to 4 completely distinct living rooms/dioramas** can fit into flash simultaneously.

---

## 4. Scaling Pixel Art Without Distortion

Embedded displays render pixels 1:1 with square physical pixels ($1:1$ aspect ratio). Sprites are **never stretched or distorted** unless non-uniform scaling code is explicitly introduced.

### Nearest-Neighbor Integer Scaling ($1\times, 2\times, 3\times$)
* **Never use bilinear or fractional scaling** (e.g. $1.35\times$), as it produces blurry lines and uneven pixel grid artifacts.
* **Always scale by whole integers** using nearest-neighbor interpolation:
  * A $48\times 48$ sprite scaled **$2\times$** becomes $96\times 96$ pixels ($40\%$ of screen width).
  * A $48\times 48$ sprite scaled **$3\times$** becomes $144\times 144$ pixels ($60\%$ of screen width).
  * Lines remain razor-sharp and chunky with authentic retro aesthetic.

### Two Ways to Scale:
1. **Tool-Side Pre-Scaling (`slice_spritesheet.py --scale 2`)**:
   - Frames are saved at the scaled resolution ($96\times 96$).
   - *Advantage*: Fastest rendering, zero CPU cycles on the ESP32.
   - *Tradeoff*: Uses 4× the flash memory per frame.
2. **Firmware-Side Dynamic Scaling**:
   - Store frames at native low resolution ($48\times 48$) in flash.
   - The ESP32 draws each pixel as a $2\times 2$ block of screen pixels on the fly.
   - *Advantage*: Saves 75% flash storage while still filling the screen.
   - *Tradeoff*: Minimal CPU overhead (takes <1.5ms on a 240MHz dual-core CPU).

---

## 5. Avenues for Further Flash Expansion

If Wilhelm's collection of animations grows beyond the current 714 KB headroom, two clean architectural avenues exist:

### A. Expand `app0` in `firmware/partitions.csv`
Currently:
```csv
app0,   app,  ota_0,   0x10000, 0x200000,   # 2.0 MB
spiffs, data, spiffs,  0x210000,0x1E0000,   # 1.875 MB
```
The companion dashboard bundle only takes **~556 KB** in `spiffs`. We can safely allocate **2.5 MB to `app0`** and **1.4 MB to `spiffs`**:
```csv
app0,   app,  ota_0,   0x10000, 0x280000,   # 2.5 MB (adds ~512 KB to firmware!)
spiffs, data, spiffs,  0x290000,0x160000,   # ~1.4 MB (still has >800 KB free for dashboard)
```
This instantly adds **~512 KB of flash space**, allowing **~60 more character animations** or **3 more full-screen backgrounds**.

### B. Store Compressed GIFs in LittleFS
The `spiffs` (LittleFS) partition currently has **1.36 MB of completely unused flash**.
* Because pixel art uses a restricted palette (16–32 colors) and large flat color regions, an animated `.gif` compresses by **80%–90%** compared to raw RGB565.
* A 6-frame $64\times 64$ pixel art GIF is typically only **4 KB to 8 KB in total file size** (compared to 49 KB raw).
* By decoding GIFs from LittleFS using an embedded decoder (e.g. `AnimatedGIF`), **over 150 animations** can be stored in the remaining 1.36 MB without touching firmware code space.

---

## 6. Implementation Status & Planned Optimization Roadmap

To keep development agile, optimizations are scheduled progressively as artwork is introduced:

### Current Implementation Status
| Feature / Optimization | Status | Notes |
| :--- | :---: | :--- |
| **Zero-RAM Compilation (`PROGMEM`)** | ✅ **Active** | `build_sprites.py` emits flash arrays; 0 bytes dynamic RAM used. |
| **Automated SCons Build Hook** | ✅ **Active** | Recompiles sprites before PlatformIO builds without manual C++ edits. |
| **Multi-Tier Spritesheet Slicer** | ✅ **Active** | `slice_spritesheet.py` with variable columns, presets, and scaling. |
| **RGB565 Raw Conversion** | ✅ **Active** | 16-bit color mapped from 8-bit RGBA. |
| **Automatic Transparency Bounding-Box Cropping** | ⏳ *Planned* | Slices empty transparent padding off foreground overlays (Strategy A). |
| **Indexed Palette Compression (4-bit/8-bit)** | ⏳ *Planned* | Reduces pixel data by 50% to 75% using color lookup tables (Strategy B). |
| **LittleFS PNG/GIF Streaming Decoder** | ⏳ *Planned* | Decodes compressed PNG/GIF files directly from LittleFS flash (Strategy C). |

### Planned Compression Strategies (Detailed)

#### Strategy A: Automatic Bounding-Box & Transparency Stripping
* **The Problem**: In foreground overlays like `desk_foreground.png`, the entire upper portion of the screen above the desk surface is transparent. Saving a full $240 \times 320$ array wastes over $100\text{ KB}$ storing invisible pixels.
* **The Solution**: Update `build_sprites.py` to calculate the non-transparent pixel bounding box `(min_x, min_y, max_x, max_y)`. Only the active rectangle (e.g. $240 \times 90$) is converted to byte data, alongside its `(offset_x, offset_y)` placement coordinates.
* **Projected Savings**: Slashes foreground overlays from **$153.6\text{ KB}$ down to $\approx 25\text{ KB}$** (an **80%+ reduction**).

#### Strategy B: Indexed Color / Palette Compression (4-bit & 8-bit)
* **The Problem**: Raw RGB565 allocates 16 bits (2 bytes) per pixel even though pixel art typically uses only 16 to 32 unique colors.
* **The Solution**: 
  1. Extract a small palette lookup table (e.g. 16 colors = 32 bytes; 256 colors = 512 bytes).
  2. Store each pixel as an index pointer: **4 bits** (0.5 byte/pixel) or **8 bits** (1 byte/pixel).
  3. The display engine expands the index via palette lookup during SPI DMA transfer.
* **Projected Savings**:
  * 8-bit Indexed: **$76.8\text{ KB}$** per full-screen frame (50% reduction).
  * 4-bit Indexed: **$38.4\text{ KB}$** per full-screen frame (75% reduction).

#### Strategy C: Direct LittleFS Compressed Asset Streaming
* **The Problem**: Firmware binary space in `app0` has a 2.0MB partition ceiling.
* **The Solution**: Place raw `.png` and `.gif` files directly into LittleFS (`firmware/data/scenes/`), which currently has **1.36 MB of completely unused flash**. Use an embedded streaming decoder (e.g. `AnimatedGIF` or `PNGdec`) to decompress frames directly into the ST7789 display buffer on the fly.
* **Projected Savings**: Allows **over 30 complete full-screen animated living rooms** to fit on the device without touching code flash.

