# 2026-09-07: Mascot Scene Selector, Clean Diegetic Face 3, & Symmetrical Simulator Chassis Geometry

## 1. Context & Motivation
During interactive testing of Face 3 (the Mascot Face) using temporary diorama scenes located in `art/Scaled Down/`, three usability and fidelity issues were identified:
1. **Scene Title Clutter on Screen**: Face 3 previously rendered a semi-transparent HUD pill overlay at the top showing the scene name and index (`sc->title (idx + 1 / count)`), which detracted from the immersive full-screen pixel-art diorama.
2. **Missing Mascot Idle Selector in Simulator**: While physical gestures (Tap for next scene, Shake for previous scene) worked, developers and artists in the Digital Twin Workbench had no direct UI controls to preview and switch between the 10 mascot scenes on demand.
3. **Simulator Chassis Geometry & Screen Realism**:
   - The simulator display panel mistakenly used rounded corners (`rounded-xl` and `rounded-lg`) with inner black bezel padding (`p-1 bg-black`), whereas the actual physical ST7789 IPS LCD panel and wooden cutout have sharp 90° corners with the image reaching the cutout edge-to-edge.
   - The wooden companion cube chassis in the simulator was previously 360×360px and used flex column alignment with the `"K U B I"` text placed below the screen. This resulted in an off-center screen with a lopsided vertical margin: the top margin was only ~10px while the bottom was ~30px. When rotated 180° on Face 3, this made the bottom screen edge appear dangerously close (~5–10px) to the chassis perimeter.

## 2. Key Architecture & Design Decisions

### A. Clean Diegetic Mascot Rendering (Firmware & Sim)
- In [`firmware/src/DisplayManager.cpp`](file:///d:/Developer/Kubi/firmware/src/DisplayManager.cpp#L380-L415), removed the top scene title pill overlay entirely.
- The 240×320 scene RLE pixel art now renders uninterrupted edge-to-edge.
- The floating golden temperature badge pill (`22.0 °C`) at the bottom is preserved (`cx - 50, h - 36, 100, 28, 8`) as an anchor badge.

### B. Two-Way Synchronized Mascot Scene Selector (Workbench UI)
- In [`dashboard/src/pages/Simulator.tsx`](file:///d:/Developer/Kubi/dashboard/src/pages/Simulator.tsx), added a dynamic **Mascot Idle Scene Selector** section that mounts between **Cube Orientation (Face Up Selector)** and **Physical Manipulation Gestures** specifically when Face 3 is active (`activeFace === 2`).
- Provides 10 responsive scene buttons matching the scenes compiled by `firmware/tools/build_scenes.py`:
  1. Beach Ice Cream
  2. Birthday Party
  3. Christmas Holiday
  4. Flower Field
  5. Listening to Music
  6. Sitting by Cafe
  7. Sleeping in Bed
  8. Watching Sunset
  9. Working from Home
  10. Gym Workout
- Selecting any scene card sends `POST /sim/inject` with `{"mascotScene": index}`.
- Telemetry heartbeat polling (`GET /sim/state` and `GET /api/state`) reports the active `mascotScene`, keeping the highlighted card in two-way sync when cycling scenes via physical tap and shake gestures.

### C. Sharp Corners & Symmetrical Chassis Scaling (1:1 Cube Ratio)
- **Sharp Display Cutout**: Removed `p-1 bg-black`, `rounded-xl`, and `rounded-lg` from the display container and canvas. The canvas now fills the sharp 240×320 window directly with `border-2 border-zinc-800`.
- **Symmetrical 400×400px Chassis**:
  - Scaled the square chassis from 360×360px to 400×400px.
  - Centered the 244×324px framed display dead-center within the chassis (`flex items-center justify-center`).
  - Vertically: $(400 - 324) / 2 = 38\text{px}$ margin at the top, and $38\text{px}$ margin at the bottom.
  - Horizontally: $(400 - 244) / 2 = 78\text{px}$ margin on left and right.
  - The `"K U B I"` branding text is positioned absolutely inside the bottom margin (`absolute bottom-3 left-0 right-0`), giving balanced spacing above and below the text without offsetting the screen.
  - Across all 4 orientations (including 180° rotation on Face 3), the screen is perfectly balanced with equal margins on the short edges.

## 3. Code Touchpoints & Files
- [`firmware/src/DisplayManager.cpp`](file:///d:/Developer/Kubi/firmware/src/DisplayManager.cpp): Removed scene title overlay pill in `drawMascotFace()`.
- [`firmware/src/API.cpp`](file:///d:/Developer/Kubi/firmware/src/API.cpp): Exposed `mascotScene` in `GET /api/state` and `POST /api/settings`.
- [`firmware/sim/src/sim_main.cpp`](file:///d:/Developer/Kubi/firmware/sim/src/sim_main.cpp): Handled `mascotScene` in `POST /sim/inject`, `GET /sim/state`, and REST endpoints.
- [`dashboard/src/pages/Simulator.tsx`](file:///d:/Developer/Kubi/dashboard/src/pages/Simulator.tsx): Added 10-button mascot scene selector, removed canvas padding and rounded corners, scaled chassis to 400×400px, and centered screen with equal 38px margins.
- [`firmware/tools/build_scenes.py`](file:///d:/Developer/Kubi/firmware/tools/build_scenes.py) & [`firmware/platformio.ini`](file:///d:/Developer/Kubi/firmware/platformio.ini): Automated pre-build asset conversion pipeline for 10 scenes from `art/Scaled Down/`.

## 4. Gotchas & Verification
- **Flexbox Centering vs. Relative Offsets**: Placing branding text in standard flex flow below the canvas offsets the screen upwards, creating severe lopsidedness when the chassis rotates 180°. Positioning branding absolutely in the margin ensures the screen remains centered at $(0, 0)$ relative to the cube centroid.
- **Physical ST7789 Geometry**: Desktop simulator canvases often default to CSS rounded borders; always keep the simulated active matrix display sharp (`rounded-none`) to match real hardware acrylic/glass cutouts.
- **Verification Commands**:
  - Firmware Silicon Build: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run`
  - Frontend Dashboard Build: `npm run build` in `dashboard/`
  - Simulator Clang++ Native Build: verify via `firmware/sim/build_sim.ps1`
