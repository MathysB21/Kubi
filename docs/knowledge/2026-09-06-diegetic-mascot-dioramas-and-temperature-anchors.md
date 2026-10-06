# 2026-09-06: Diegetic Mascot Dioramas and Contextual Temperature Anchors

## 1. Context & Motivation
On Face 3 (Mascot & Room Temp), the display was originally divided into two separate functional zones:
- An upper canvas rendering an animated or vector-drawn mascot.
- A lower footer area rendering a generic digital temperature readout (`"24.5 °C" / "Room Temperature"`) against a black background.

While functional, this split layout left significant unused black margins and broke visual immersion. The user proposed a radical aesthetic upgrade: **fully utilizing every single pixel of the $240\times 320$ screen** by turning it into a rich scene (e.g. an office, bedroom, or coffee shop), and embedding the live temperature reading directly into the scene itself (such as onto a physical wooden calendar block on Kubi's desk).

## 2. Key Architecture & Design Decisions

### 2.1 The Diegetic UI Philosophy
In game and narrative design, a **diegetic user interface** exists naturally inside the story world rather than as an artificial floating heads-up display (HUD). For Kubi, this means:
- **Office / Desk Scene**: Temperature appears printed onto a miniature wooden block or digital desk clock sitting next to Kubi's laptop.
- **Sleep / Night Scene**: Temperature glows faintly on a bedside nightstand alarm clock or wall thermometer.
- **Coffee / Morning Scene**: Temperature is written in chalk on a kitchen blackboard menu or coffee counter sign.

### 2.2 Layered Scene Composition (Solving the Flash Memory Ceiling)
A full-screen $240\times 320$ frame in 16-bit RGB565 takes $240 \times 320 \times 2 = 153,600\text{ bytes} \approx 150\text{ KB}$.  
If a 5-frame animation re-rendered the entire room on every frame, it would consume $5 \times 150\text{ KB} = 750\text{ KB}$, instantly exhausting the entire ~714 KB free firmware flash headroom.

Instead, scenes are structured into **3 layered passes**:
1. **Layer 1: Static Full-Screen Background ($240\times 320$)**: 1 single frame ($150\text{ KB}$). Draws the room walls, floor, bookshelf, desk, lamp, and wooden temperature block.
2. **Layer 2: Animated Mascot Sprite Overlay ($64\times 64$ to $80\times 80$)**: 4–6 animated frames ($\approx 20\text{ KB}$). Only Kubi's animated body, hands, and expressions are stored and blitted onto the scene at coordinates $(X_{\text{mascot}}, Y_{\text{mascot}})$.
3. **Layer 3: Diegetic Temperature Anchor**: Text rendering pass ($0\text{ KB}$). Live BMP280 temperature is drawn directly on top of the wooden block at $(X_{\text{temp}}, Y_{\text{temp}})$ using configured typography, color, and alignment.

**Net Result**: A full-screen, edge-to-edge diorama consumes only **$\approx 170\text{ KB}$**, enabling **3 to 4 rich environments** to fit simultaneously in flash.

### 2.3 Scene & Temperature Anchor Metadata Schema
Scenes pair visual assets with contextual anchor coordinates:
```cpp
struct TemperatureAnchorConfig {
    bool enabled;
    int16_t x;            // Exact pixel X on the wooden block (e.g. 184)
    int16_t y;            // Exact pixel Y on the wooden block (e.g. 220)
    uint8_t font;         // TFT font (1: 5x7 tiny, 2: 16px crisp, 4: 26px large)
    uint16_t textColor;   // RGB565 color (e.g. 0x0000 black, 0xFD20 orange, 0xCE79 wood)
    uint16_t bgColor;     // Matching background color of the block (or transparent)
    uint8_t datum;        // Alignment: MC_DATUM (middle-center), TC_DATUM, etc.
    const char* format;   // "%.1f°C", "%.0f°", or "%.1f"
};

struct MascotDioramaScene {
    const char* name;
    const uint16_t* background240x320;
    const KubiSpriteAnimation* mascotAnim;
    int16_t mascotX;
    int16_t mascotY;
    TemperatureAnchorConfig tempAnchor;
};
```

## 3. Code Touchpoints & Files
- [`firmware/src/DisplayManager.h`](file:///d:/Developer/Kubi/firmware/src/DisplayManager.h) & [`firmware/src/DisplayManager.cpp`](file:///d:/Developer/Kubi/firmware/src/DisplayManager.cpp): `drawMascotFace()` rendering pipeline and text datum positioning.
- [`firmware/include/KubiSprites.h`](file:///d:/Developer/Kubi/firmware/include/KubiSprites.h): Animation struct declarations and future diorama scene metadata bindings.
- [`docs/Animation and Storage Optimization Guide.md`](file:///d:/Developer/Kubi/docs/Animation%20and%20Storage%20Optimization%20Guide.md): Reference guide for memory formulas, partition ceilings, and layered diorama budgets.

## 4. Gotchas & Verification
1. **Font Sizing & Wooden Block Bounding Box**:
   - The wooden block drawn into the background image must have sufficient width and height to accommodate the rendered text without overflow.
   - For Font 2 (`16px` high), a string like `"23.5°C"` requires $\approx 55\text{px} \times 16\text{px}$. The drawn wooden block should be at least $65\text{px} \times 24\text{px}$.
2. **Text Background Matching**:
   - To avoid screen flicker when temperature values update, draw with a specified background color (`_tft.setTextColor(textColor, blockBgColor)`) rather than transparent text, ensuring old digits are cleanly overwritten.
3. **Interactive SITL Simulator Coordinate Tuning**:
   - Use the Desktop SITL Digital Twin (`http://localhost:5173/sim`) to verify pixel alignment: clicking or hovering over the rendered diorama provides instantaneous $(X, Y)$ coordinate feedback to position the temperature anchor with single-pixel accuracy.
