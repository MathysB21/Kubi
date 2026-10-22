# Kubi enclosure

Parametric OpenSCAD model of the 90 mm cube: `kubi_enclosure.scad`. Open it in OpenSCAD (2021 or newer), set `part` in the Customizer, press F6 to render, then File > Export > STL. Every part comes out already in its print orientation.

## Parts

| Part | Print | Notes |
|---|---|---|
| `front` | 1, front face down | Screen window, speaker grilles, four corner posts with the magnet ledges, screw blocks on top of the posts |
| `back` | 1, back face down | USB-C port, vents, four legs that carry the mid-plate |
| `midplate` | 1, battery side down | Battery holder strapped in front, perfboard on standoffs behind |
| `side` | 4, outer face down | Removable panels, magnets on the inside |

`assembly` and `exploded` are for looking at it, not for printing.

## Measure before printing

These are parameters at the top of the file, set to sensible defaults:

- `mag_d`, `mag_h`: your magnets. The pockets add `mag_fit` (0.3 mm) to the diameter for a press fit; glue them anyway.
- `view_dx`, `view_dy`: where the screen's visible area sits on its PCB. Measure the visible area's centre against the PCB's centre with the module as it sits on Face 1 (portrait, cable plug at the back). Without this the window can be off-centre on the picture.
- Perfboard: its 2 mm corner holes are imprecise, so the mid-plate has solid 7 mm pads (`pb_pilot = 0`) instead of fixed screw holes. Either glue the board to the pads, or hold it in place, mark through its holes, drill 1.6 mm and use M2 screws.
- Optional: `spk_x`, `scr_y`, `usb_x`, `usb_y` to move things.

## Print settings (PETG)

- 0.2 mm layers, 4 walls, 4 top/bottom layers, 20 % gyroid infill.
- No supports needed. The screw blocks have 45 degree undersides and the rest is vertical.
- Brim on `front` and `back`: the rounded edge meets the bed at a steep angle and the posts and legs are tall and thin.
- Test print one `side` panel first to check the fit (`gap`, 0.25 mm per side) and the magnet pockets.

## Hardware

- 8 x M3 x 10 countersunk screws: 4 hold the back on (into the posts), 4 hold the mid-plate onto the legs. They self-tap into the 2.6 mm pilot holes.
- 4 x M2 x 5 screws for the perfboard (drill the pads to 1.6 mm first), or glue.
- 2 zip ties up to 4 mm wide, around the battery holder.
- 32 magnets: 4 per panel plus 4 in the frame behind each panel. With fewer, use two per panel on opposite corners.
- Superglue or epoxy for the magnets, hot glue for the TP4056 and loose wires.

## Magnets

Glue every frame magnet with the same pole facing out, and every panel magnet with the opposite pole facing out. Mark one side of a test magnet and check each one against it before gluing. Then any panel fits any face, either way round.

They do not affect the electronics: the ESP32, flash, screen and amp are not magnetic, the ADXL345 measures acceleration rather than magnetism, and the compass chip on the sensor board is not used. They do tug on steel: keep loose screws and the cells away while gluing.

## Assembly

1. Front: screen into its brackets behind the window, cable plug towards the inside. Speakers into their brackets either side, sound openings facing the grilles. Hot glue to hold.
2. Mid-plate: battery holder on the front (the countersunk side), strapped with two zip ties through the slots. Perfboard on the standoffs at the back.
3. Back: TP4056 standing on its edge with the USB-C receptacle pushed into the slot until it is flush with the outside, then glue it generously (it takes the force of every plug-in). MT3608 and PAM8403 flat on the inside of the back plate.
4. Screw the mid-plate onto the back's legs (4 x M3, from the battery side).
5. Wire everything as in `docs/Kubi Assembly Guide.md`, then lower the back assembly into the front body and screw it on (4 x M3 into the posts).
6. Press the side panels in.
7. Recalibrate the faces (`firmware/tools/capture_faces.ps1`).

## Layout (cube on Face 1, screen towards you)

- Front 0-3 mm: face; 3-~13 mm: screen module and its plug, speakers either side.
- 14-35 mm: battery holder (78 mm side to side), centred, so the cube rolls evenly.
- 35-38 mm: mid-plate. 42-~62 mm: perfboard with the ESP32 and IMU.
- Back plate inside: TP4056 below centre (USB-C at x 0, y -30), MT3608 and PAM8403 anywhere clear of the legs at (±29, ±29). Do not put the PAM8403's knob (about 20 mm tall) straight above the upright IMU on the perfboard: with the back held in place, check there are a few millimetres between them before gluing.
