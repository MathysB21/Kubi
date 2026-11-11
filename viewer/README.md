# Kubi build viewer

A local React + Vite + three.js app for building the cube: the printed enclosure in 3D with an exploded view, and the electronics: where every part sits in the cube, the perfboard hole by hole, and the assembly steps as a checklist.

```powershell
cd viewer
npm install
npm run dev     # http://localhost:5174
```

## Tabs

- **Enclosure**: the four printed parts, loaded from `../enclosure/stl/assembly/*.stl`. Explode slider and Explode / Assemble buttons, part toggles, the 32 magnets, and print notes per part (click a part).
- **Electronics**
  - **Fit in the cube (3D)**: every component inside a see-through enclosure, with the wires routed through the gaps round the mid-plate. Exploding pulls the layers apart along the roll axis and the wires stretch with them. Click a part for its size, position, connections and notes; hover a wire to trace it.
  - **Perfboard & wiring**: the 14 x 20 board, component side or solder side (mirrored, for wiring underneath), ESP32 socket pins, headers, R2, and every wire coloured by net. Hover to trace, click to tick off; the checklists also cover the off-board wiring.
  - **Assembly steps**: the four stages from `docs/Kubi Assembly Guide.md` with their checks. Ticks are saved in the browser.

## Where the numbers come from

Everything is in the OpenSCAD model's frame (cube on Face 1, screen at the front, z from the front face to the back), so the STLs, components and wires line up:

- `src/model/enclosure.ts`: the printed parts and magnet positions (from `enclosure/kubi_enclosure.scad`).
- `src/model/electronics.ts`: component positions and sizes, and the 3D wire routes.
- `src/model/perfboard.ts`: socket pins, pads and the solder-side wire list (from `docs/Kubi Architecture and Wiring Guide.md`, section 5).
- `src/model/steps.ts`: the assembly stages.

When the enclosure changes, re-export the assembled STLs from OpenSCAD (`part = "front_asm"`, `"back_asm"`, `"midplate_asm"`, `"side_asm"`) into `enclosure/stl/assembly/`. When the wiring changes, update the guide and `perfboard.ts` together.

Wire routes inside the cube and the screen's plug position are approximate; the perfboard plan is exact.
