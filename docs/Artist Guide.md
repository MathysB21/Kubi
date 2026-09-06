# Kubi Artist & Animation Guide

Welcome to the **Kubi Artist Guide**! This document provides visual artists, pixel designers, and animators with all the canvas dimensions, layering techniques, and best practices needed to create living scenes and animations for **Project Kubi**.

---

## 1. Screen & Canvas Specifications

The physical screen inside Kubi is a **2.0" IPS LCD (ST7789)**.

* **Resolution (Face 3 Mascot Mode)**: **$240\text{ pixels wide} \times 320\text{ pixels tall}$** (Portrait, 3:4 aspect ratio).
* **Pixel Aspect Ratio**: **$1:1$ (Square Pixels)**.
  * The screen renders pixels with 1:1 physical square geometry. Your artwork will **never be stretched, squeezed, or distorted** out of proportion.
* **Color Depth**: **16-bit RGB565** (65,536 colors).
  * Best practice for pixel art: stick to a cohesive **16 to 32 color palette** for that authentic retro arcade aesthetic (and hyper-efficient compression).

```
        0 px                               240 px
   0 px ┌──────────────────────────────────────┐
        │                                      │
        │                                      │
        │          UPPER BACKGROUND            │
        │        (Walls, Windows, Art)         │
        │                                      │
        │                                      │
        │             CHARACTER                │
        │               STAGE                  │
 160 px ├ - - - - - - - - - - - - - - - - - - -┤
        │                                      │
        │         FOREGROUND FURNITURE         │
        │          (Desk, Table, Props)        │
        │                                      │
 320 px └──────────────────────────────────────┘
```

---

## 2. Working Canvas: Native vs. Retro Upscaling

You have two great ways to set up your project in Aseprite, Photoshop, or your pixel editor:

### Option A: Native $240 \times 320$ Canvas (Recommended for Dioramas)
* Create your canvas at **$240\text{w} \times 320\text{h}$**.
* Every pixel you place corresponds to exactly 1 physical pixel on Kubi's screen.
* *Best for*: Detailed room scenes, crisp linework, small props, and readable text blocks.

### Option B: Retro Half-Resolution ($120 \times 160$) with $2\times$ Integer Scaling
* Create your canvas at **$120\text{w} \times 160\text{h}$** (half size).
* Draw with chunky pixels, then export at **$2\times$ scale (Nearest-Neighbor)** to reach $240 \times 320$.
* *Best for*: Chunky Game Boy Advance / SNES aesthetic, and faster illustration time (75% fewer pixels to draw!).

> [!IMPORTANT]
> **Nearest-Neighbor Scaling Only**:  
> Never export pixel art using bilinear or bicubic filtering (which creates blurry, smudged edges). Always select **Nearest Neighbor** integer scaling ($1\times, 2\times, 3\times$).

---

## 3. Scale & Proportions Inside the Room

When designing a full-screen diorama, use these proportions as your visual compass:

| Scene Element | Recommended Width | Recommended Height | Description & Placement |
| :--- | :--- | :--- | :--- |
| **Room Background** | **$240\text{px}$** | **$320\text{px}$** | Fills the entire screen edge-to-edge. |
| **Kubi Character** | **$60\text{ to }90\text{px}$** | **$60\text{ to }80\text{px}$** | $\approx 25\% - 35\%$ of screen width. Sits or stands in the center stage. |
| **Furniture (Desk)** | **$200\text{ to }240\text{px}$** | **$80\text{ to }120\text{px}$** | Spans across the lower third of the canvas ($y \approx 200 \dots 320$). |
| **Wooden Temp Block** | **$\ge 55\text{px}$** | **$\ge 20\text{px}$** | Flat surface on the desk where temperature text is painted. |

---

## 4. The 4-Layer Sandwich Technique (Z-Ordering)

When Kubi is sitting behind a desk or table, you don't need to manually cut his legs off frame-by-frame! Instead, organize your art into a **4-layer sandwich**:

```
[Layer 1: Background]       ──► Walls, windows, bookshelf, back of the chair (Opaque)
          ▲
[Layer 2: Mascot Anim]      ──► Animated Kubi sitting in the chair (Transparent background)
          ▲
[Layer 3: Foreground Desk]  ──► Desk, laptop, coffee cup, wooden block (Transparent above desk)
          ▲
[Layer 4: Temperature UI]   ──► Live room temp text painted right onto the wooden block!
```

### How to Export Your Layers:
1. **`background.png` ($240 \times 320$, Opaque)**:
   * Contains the room walls, floor, posters, ceiling lights, and the back of Kubi's chair.
2. **`kubi_anim.gif` (or folder of PNGs, Transparent background)**:
   * Just Kubi himself (typing, blinking, waving).
   * Draw Kubi's full body naturally. You don't have to clip his lower torso—the desk layer will cover it cleanly!
3. **`foreground_desk.png` (Transparent background)**:
   * Contains the desk, laptop, coffee mug, and the wooden block.
   * Everything above the desk surface is **100% transparent** so Kubi and the background show through.
   * *Tip*: You only need to export the desk strip (e.g. $240\text{w} \times 100\text{h}$), which saves massive memory.

---

## 5. Diegetic UI: Designing the Temperature Anchor

**Diegetic UI** means interface elements look like natural physical objects inside the story world rather than a floating HUD:

```
                  ┌──────────────────────┐
                  │    HAPPY BIRTHDAY    │
                  │       WILHELM        │
                  └──────────┬───────────┘
                             │
                      ┌──────┴─────┐
                      │    KUBI    │
                      │  (typing)  │
      ┌───────────────┴────────────┴──────────────────────────┐
     /   [Laptop]                     ┌──────────────────┐     \
    /     ┌───┐                       │     23.5 °C      │      \
   /      └───┘                       │  [WOODEN BLOCK]  │       \
  /                                   └──────────────────┘        \
 └─────────────────────────────────────────────────────────────────┘
```

### Guidelines for the Wooden Block:
1. **Flat Surface Area**: Leave a flat, uncluttered zone on the block of at least **$55\text{px wide} \times 20\text{px tall}$** so text like `"23.5°C"` fits with breathing room.
2. **High Contrast**: If your wooden block is dark brown (`#4A2511`), choose a warm cream/beige text color (`#FDF0D5`). If the block is light birch (`#DDB892`), choose dark charcoal text (`#1C1917`).
3. **Record Coordinates**: Note down the $(X, Y)$ pixel coordinate of the center of your wooden block (e.g. `X: 182, Y: 225`).

---

## 6. Animation Tips & Timing

* **Frame Rate (FPS)**: For charming retro pixel art, aim for **6 to 10 FPS** (a frame delay of **$100\text{ms}$ to $160\text{ms}$** per frame).
* **Frame Count**: **4 to 6 frames** per animation loop is the sweet spot:
  * **Typing Loop**: 4 to 6 frames of rhythmic hand movement.
  * **Walking Loop**: 6 frames (left step, pass, right step, pass).
  * **Idle / Breathing**: 4 frames (gentle 1–2 pixel squash and stretch).
* **Seamless Loops**: Ensure the final frame transitions smoothly back into Frame 0 without jump cuts.

---

## 7. Using the Slicing Tool (`slice_spritesheet.py`)

If you generate or draw multi-row spritesheets (even with different numbers of columns per row), you can slice them with our automated tool:

```powershell
# Built-in preset for Wilhelm's 4-tier Kubi sheet (Idle 4 cols, Walk 6 cols, Wave 4 cols, Desk 5 cols):
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/slice_spritesheet.py `
  -i "path/to/sheet.png" `
  --preset wilhelm

# Custom row layout:
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" firmware/tools/slice_spritesheet.py `
  -i "sheet.png" `
  -r "idle:4,walk:6,wave:4,desk:5" `
  --scale 2
```

---

## 8. Where to Save Your Finished Assets

When your artwork is ready, organize it into:

```
firmware/assets/scenes/
└── office/
    ├── background.png         # Full 240x320 room
    ├── foreground_desk.png    # The desk with transparent upper area
    ├── kubi_typing.gif        # Kubi animation (or folder of PNG frames)
    └── scene.json             # Placement coordinates
```

### Example `scene.json`:
```json
{
  "name": "office_birthday",
  "mascot": {
    "x": 85,
    "y": 140
  },
  "temperature_anchor": {
    "x": 182,
    "y": 224,
    "font": 2,
    "color": "#FDF0D5",
    "bg_color": "#4A2511"
  }
}
```

---

## 9. Visual Verification in the Simulator

You don't need physical hardware to see your art live on Kubi:
1. Open the **Digital Twin Simulator Workbench**: `http://localhost:5173/sim`.
2. Rotate the simulated wooden cube to **Face 3 (Mascot)**.
3. Your background, animated Kubi, and the live temperature badge will appear inside the 3D cube chassis on your monitor exactly as it will look on Wilhelm's desk!
