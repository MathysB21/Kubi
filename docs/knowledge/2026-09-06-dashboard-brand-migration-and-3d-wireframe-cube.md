# 2026-09-06: Dashboard Brand Migration, Custom Favicon, and 3D Wireframe Cube

## 1. Context & Motivation
- Residual assets and naming from the predecessor project ("Sunrise") lingered in the companion web dashboard (specifically the browser document `<title>` showing `S U N R I S E`, legacy sun favicon, and the header `SkyArc` element).
- The dashboard was refreshed to align 100% with the Kubi brand identity:
  - The document page title was rebranded to `Kubi`.
  - The favicon was migrated to the user's `box.svg` icon while preserving the legacy signature orange (`rgb(254, 154, 0)`).
  - The astronomical `SkyArc` countdown widget was removed and replaced with an interactive 3D perspective wireframe companion cube directly above the Kubi header.

## 2. Key Architecture & Design Decisions

### 3D Perspective Wireframe Cube ([SpinningCube.tsx](file:///c:/Development/Kubi/dashboard/src/components/SpinningCube.tsx))
- **Zero Heavy 3D Dependencies**: Implemented using pure HTML5 2D Canvas vector math rather than importing Three.js or WebGL libraries, keeping the dashboard bundle extremely lightweight (~481 KB gzipped to ~150 KB).
- **Geometry & Matrix Projection**:
  - 8 vertices normalized between $[-1, 1]$ and 12 connecting edges.
  - Multi-axis rotation (continuous slow delta on $Y$, $X$, and $Z$).
  - Gentle vertical sine bobbing ($bobOffset = \sin(bobTime) \times 6$).
- **Color Styling**:
  - Edges and glow styled with Kubi's primary `amber-500` palette (`#F59E0B` / `rgb(245, 158, 11)`), harmonizing with button text and UI accents.
  - Ambient glow created using layered canvas strokes (soft $4.0\text{px}$ glow under crisp $1.8\text{px}$ core stroke).
  - Minimalist wireframe: No solid faces or corner vertex points.
- **Physical Floor Shadow Physics**:
  - Positioned at $y = 138$ on a $165\text{px}$ canvas to guarantee zero boundary clipping.
  - Mapped to downward vertical coordinates: $proximity = (bobOffset + 6) / 12 \in [0, 1]$.
  - When cube is at its lowest point (closest to floor): shadow tightens to a $32\text{px}$ radius with crisp, darker opacity ($0.30$).
  - When cube floats up: shadow expands to a $48\text{px}$ radius with diffused, softer opacity ($0.12$).
  - Uses a scaled canvas matrix context (`ctx.scale(1, radiusY / radiusX)`) with a multi-stop exponential radial falloff to eliminate hard edge cuts.

### Favicon & Page Title Synchronization
- `dashboard/index.html` updated with `<title>Kubi</title>`.
- `dashboard/public/favicon.svg` created from `box.svg` with `stroke="rgb(254, 154, 0)"`.
- Running `npm run build` bundles the distribution into `firmware/data/`, automatically syncing `index.html` and `favicon.svg` into the LittleFS filesystem for the physical ESP32 web server.

## 3. Code Touchpoints & Files
- [dashboard/index.html](file:///c:/Development/Kubi/dashboard/index.html): Page title changed to "Kubi".
- [dashboard/public/favicon.svg](file:///c:/Development/Kubi/dashboard/public/favicon.svg): Updated favicon using the box vector and legacy brand color.
- [dashboard/src/components/SpinningCube.tsx](file:///c:/Development/Kubi/dashboard/src/components/SpinningCube.tsx): Canvas-based 3D spinning and bobbing wireframe cube with dynamic floor shadow.
- [dashboard/src/components/SkyArc.tsx](file:///c:/Development/Kubi/dashboard/src/components/SkyArc.tsx): Removed.
- [dashboard/src/App.tsx](file:///c:/Development/Kubi/dashboard/src/App.tsx): Replaced SkyArc with SpinningCube above the header.
- [firmware/data/](file:///c:/Development/Kubi/firmware/data/): Recompiled production bundle for ESP32 LittleFS.

## 4. Gotchas & Verification
- **Canvas Vertical Coordinate System**: Remember that in HTML5 Canvas, $y$ increases downwards. Always ensure bobbing/distance-to-floor calculations invert $y$ or account for direction to avoid inverted shadow physics.
- **Radial Gradients with Ellipses**: Applying a standard 2D radial gradient to an ellipse will cause circular opacity falloff that cuts off vertically before horizontally. Always isolate in a `ctx.save() / ctx.scale(1, radiusY / radiusX) / ctx.restore()` block.
- **Verification**:
  - `npm run build` in `dashboard/` to verify TypeScript types and asset bundling.
  - `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run` in `firmware/` to ensure firmware integrity.
